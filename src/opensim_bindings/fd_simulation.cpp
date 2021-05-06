#include "fd_simulation.hpp"

#include "src/log.hpp"
#include "src/utils/concurrency.hpp"
#include "src/utils/shims.hpp"

#include <OpenSim/Simulation/Manager/Manager.h>
#include <OpenSim/Simulation/Model/Analysis.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <SimTKsimbody.h>
#include <simmath/Integrator.h>

#include <functional>
#include <ratio>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

using namespace osc;
using sim_clock = std::chrono::steady_clock;

// status of an OpenSim simulation
enum class Sim_status {
    Running = 1,
    Completed = 2,
    Cancelled = 4,
    Error = 8,
};

// state that is shared between the simulator owner (i.e. the UI thread) and the
// simulation thread
//
// the contract here is that the simulator thread will try to update these values
// often so that the owner can monitor simulation progress.
struct Shared_fdsim_state final {
    // this will only be updated by the sim thread when it is nullptr
    //
    // a successful Fd_simulation::try_pop_state will make this into a nullptr
    std::unique_ptr<SimTK::State> latest_state = nullptr;

    sim_clock::time_point wall_start = sim_clock::now();
    sim_clock::time_point wall_end = sim_clock::now();
    Simulation_stats stats{};
    Sim_status status = Sim_status::Running;
};

// map user integrator selection to a SimTK integrator
static SimTK::Integrator* select_integrator_unguarded(SimTK::System const& system, IntegratorMethod method) {
    switch (method) {
    case osc::IntegratorMethod_OpenSimManagerDefault:
        return new SimTK::RungeKuttaMersonIntegrator{system};
    case osc::IntegratorMethod_ExplicitEuler:
        return new SimTK::ExplicitEulerIntegrator{system};
    case osc::IntegratorMethod_RungeKutta2:
        return new SimTK::RungeKutta2Integrator{system};
    case osc::IntegratorMethod_RungeKutta3:
        return new SimTK::RungeKutta3Integrator{system};
    case osc::IntegratorMethod_RungeKuttaFeldberg:
        return new SimTK::RungeKuttaFeldbergIntegrator{system};
    case osc::IntegratorMethod_RungeKuttaMerson:
        return new SimTK::RungeKuttaMersonIntegrator{system};
    case osc::IntegratorMethod_SemiExplicitEuler2:
        return new SimTK::SemiExplicitEuler2Integrator{system};
    case osc::IntegratorMethod_Verlet:
        return new SimTK::VerletIntegrator{system};
    default:
        return new SimTK::RungeKuttaMersonIntegrator{system};
    }
}

// guarded (RAII) version of the above
static std::unique_ptr<SimTK::Integrator> select_integrator(SimTK::System const& system, IntegratorMethod method) {
    return std::unique_ptr<SimTK::Integrator>{select_integrator_unguarded(system, method)};
}

static std::chrono::microseconds simulation_micros(SimTK::State const& s) {
    return std::chrono::microseconds{static_cast<long>(1000000.0 * s.getTime())};
}

struct Simulation_timers final {
    sim_clock::time_point simulation_thread_started = sim_clock::now();
    sim_clock::time_point last_report_start = sim_clock::now();
    sim_clock::time_point last_report_end = sim_clock::now();
    std::chrono::microseconds sim_initial_time;

    explicit Simulation_timers(SimTK::State const& initial_state) :
        sim_initial_time{simulation_micros(initial_state)} {
    }
};

// called per integration step
static void sim_on_integration_step(Fd_simulation_params const& params,
                                    Mutex_guarded<Shared_fdsim_state>& thread_shared,
                                    SimTK::Integrator const& integrator,
                                    Simulation_timers& timers) {

    // meta: report how long it's taking to perform these per-integration-step steps
    sim_clock::time_point report_start = sim_clock::now();

    // throttling: slow down the simulation thread if requested
    //
    // if the simulation is running faster than wall time and the caller requested that
    // it runs slower than wall time then sleep the simulation thread
    if (params.throttle_to_wall_time) {
        std::chrono::microseconds sim_time = simulation_micros(integrator.getState());
        std::chrono::microseconds sim_elapsed = sim_time - timers.sim_initial_time;
        std::chrono::microseconds wall_elapsed{
            std::chrono::duration_cast<std::chrono::microseconds>(report_start - timers.simulation_thread_started)};

        if (sim_elapsed > wall_elapsed) {
            std::this_thread::sleep_for(sim_elapsed - wall_elapsed);
        }
    }

    // inter-thread reporting
    //
    // copy any relevant information out in a threadsafe way

    Mutex_guard<Shared_fdsim_state> guard{thread_shared.lock()};
    Shared_fdsim_state& shared_st = *guard;

    // state reporting
    //
    // only copy a state if the "message space" is empty
    //
    // this is because copying a state is potentially quite expensive, so should only be
    // performed when necessary
    if (shared_st.latest_state == nullptr) {
        shared_st.latest_state = std::make_unique<SimTK::State>(integrator.getState());
    }

    // assign stats
    {
        auto& stats = shared_st.stats;

        stats.time = std::chrono::duration<double>{integrator.getTime()};

        {
            sim_clock::duration total = report_start - timers.last_report_start;
            sim_clock::duration overhead = timers.last_report_end - timers.last_report_start;
            stats.ui_overhead_estimated_ratio =
                static_cast<float>(overhead.count()) / static_cast<float>(total.count());
        }

        stats.accuracyInUse = static_cast<float>(integrator.getAccuracyInUse());
        stats.predictedNextStepSize = static_cast<float>(integrator.getPredictedNextStepSize());
        stats.numStepsAttempted = integrator.getNumStepsAttempted();
        stats.numStepsTaken = integrator.getNumStepsTaken();
        stats.numRealizations = integrator.getNumRealizations();
        stats.numQProjections = integrator.getNumQProjections();
        stats.numUProjections = integrator.getNumUProjections();
        stats.numErrorTestFailures = integrator.getNumErrorTestFailures();
        stats.numConvergenceTestFailures = integrator.getNumConvergenceTestFailures();
        stats.numRealizationFailures = integrator.getNumRealizationFailures();
        stats.numQProjectionFailures = integrator.getNumQProjectionFailures();
        stats.numUProjectionFailures = integrator.getNumUProjectionFailures();
        stats.numProjectionFailures = integrator.getNumProjectionFailures();
        stats.numConvergentIterations = integrator.getNumConvergentIterations();
        stats.numDivergentIterations = integrator.getNumDivergentIterations();
        stats.numIterations = integrator.getNumIterations();

        stats.numPrescribeQcalls = params.model->getSystem().getNumPrescribeQCalls();
    }

    timers.last_report_start = report_start;
    timers.last_report_end = sim_clock::now();
}

static int simulation_thread_main_unguarded(stop_token stop_token,
                                            Fd_simulation_params p,
                                            std::shared_ptr<Mutex_guarded<Shared_fdsim_state>> thread_shared) {
    p.model->setPropertiesFromState(*p.state);
    SimTK::State& st = p.model->initSystem();
    p.model->realizePosition(st);
    p.model->equilibrateMuscles(st);
    p.model->realizeAcceleration(st);

    // create relevant SimTK::Integrator that uses the system
    std::unique_ptr<SimTK::Integrator> integ =
        select_integrator(p.model->getMultibodySystem(), p.integrator_method);

    // these roughly match OpenSim::Manager params
    integ->setInternalStepLimit(p.integrator_step_limit);
    integ->setMinimumStepSize(p.integrator_minimum_step_size.count());
    integ->setMaximumStepSize(p.integrator_maximum_step_size.count());
    integ->setAccuracy(p.integrator_accuracy);
    integ->setFinalTime(p.final_time.count());
    integ->setReturnEveryInternalStep(p.update_latest_state_on_every_step);
    integ->initialize(st);

    // state shared during integration steps
    Sim_status simthread_status = Sim_status::Completed;
    SimTK::TimeStepper ts{p.model->getMultibodySystem(), *integ};
    ts.initialize(integ->getState());
    ts.setReportAllSignificantStates(p.update_latest_state_on_every_step);
    Simulation_timers timers{integ->getState()};

    // report state 0 *before* performing first step
    {
        sim_on_integration_step(p, *thread_shared, *integ, timers);
    }

    // start integration stepping loop
    while (integ->getTime() < p.final_time.count()) {

        // handle cancellation requests
        if (stop_token.stop_requested()) {
            simthread_status = Sim_status::Cancelled;
            break;
        }

        double next_report_time = std::min(
            integ->getTime() + p.reporting_interval.count(),
            p.final_time.count());

        // perform integration step
        auto status = ts.stepTo(next_report_time);

        // handle integration errors
        if (integ->isSimulationOver() && integ->getTerminationReason() != SimTK::Integrator::ReachedFinalTime) {
            std::string reason = integ->getTerminationReasonString(integ->getTerminationReason());
            log::error("simulation error: integration failed: %s", reason.c_str());
            simthread_status = Sim_status::Error;
            break;
        }

        switch (status) {
        // a (potentially irregular) integration step was carried out
        case SimTK::Integrator::TimeHasAdvanced:
            sim_on_integration_step(p, *thread_shared, *integ, timers);
            break;
        // the (regular) report interval time was hit
        case SimTK::Integrator::ReachedReportTime: {
            sim_on_integration_step(p, *thread_shared, *integ, timers);
            break;
        }
        default:
            break;
        }
    }

    auto guard = thread_shared->lock();
    guard->wall_end = sim_clock::now();
    guard->status = simthread_status;
    return 0;
}

// MAIN: simulator thread: this is the top-level function that the simulator thread
// executes in the background.
static int simulation_thread_main(
    stop_token stop_token,
    Fd_simulation_params p,
    std::shared_ptr<Mutex_guarded<Shared_fdsim_state>> thread_shared) {

    // ensure that the thread satisfies any shared invariants - even when
    // an exception is thrown

    try {
        return simulation_thread_main_unguarded(std::move(stop_token), std::move(p), std::move(thread_shared));
    } catch (OpenSim::Exception const& ex) {
        log::error("OpenSim::Exception occurred when running a simulation: %s", ex.what());
        auto guard = thread_shared->lock();
        guard->wall_end = sim_clock::now();
        guard->status = Sim_status::Error;
        throw;
    } catch (std::exception const& ex) {
        log::error("std::exception occurred when running a simulation: %s", ex.what());
        auto guard = thread_shared->lock();
        guard->wall_end = sim_clock::now();
        guard->status = Sim_status::Error;
        throw;
    } catch (...) {
        log::error("an exception with unknown type occurred when running a simulation (no error message available)");
        auto guard = thread_shared->lock();
        guard->wall_end = sim_clock::now();
        guard->status = Sim_status::Error;
        throw;
    }
}

osc::IntegratorMethod const osc::integrator_methods[IntegratorMethod_NumIntegratorMethods] = {
    IntegratorMethod_OpenSimManagerDefault,
    IntegratorMethod_ExplicitEuler,
    IntegratorMethod_RungeKutta2,
    IntegratorMethod_RungeKutta3,
    IntegratorMethod_RungeKuttaFeldberg,
    IntegratorMethod_RungeKuttaMerson,
    IntegratorMethod_SemiExplicitEuler2,
    IntegratorMethod_Verlet,
};

char const* const osc::integrator_method_names[IntegratorMethod_NumIntegratorMethods] = {
    "OpenSim::Manager Default",
    "Explicit Euler",
    "Runge Kutta 2",
    "Runge Kutta 3",
    "Runge Kutta Feldberg",
    "Runge Kutta Merson",
    "Semi Explicit Euler 2",
    "Verlet",
};

struct Fd_simulation::Impl final {
    std::chrono::duration<double> final_time;
    std::shared_ptr<Mutex_guarded<Shared_fdsim_state>> shared;
    jthread simulator_thread;
    int states_popped;

    Impl(Fd_simulation_params p) :
        final_time{p.final_time},

        shared{new Mutex_guarded<Shared_fdsim_state>{}},

        // starts the simulation
        simulator_thread{simulation_thread_main, std::move(p), shared},

        states_popped{0} {
    }
};

osc::Fd_simulation::Fd_simulation(Fd_simulation_params p) : impl{new Impl{std::move(p)}} {
}
osc::Fd_simulation::~Fd_simulation() noexcept = default;

std::unique_ptr<SimTK::State> osc::Fd_simulation::try_pop_state() {
    std::unique_ptr<SimTK::State> maybe_latest = std::move(impl->shared->lock()->latest_state);

    if (maybe_latest) {
        ++impl->states_popped;
    }

    return maybe_latest;
}

int osc::Fd_simulation::num_states_popped() const noexcept {
    return impl->states_popped;
}

bool osc::Fd_simulation::is_running() const noexcept {
    return impl->shared->lock()->status == Sim_status::Running;
}

std::chrono::duration<double> osc::Fd_simulation::wall_duration() const noexcept {
    sim_clock::time_point start = impl->shared->lock()->wall_start;
    sim_clock::time_point end = is_running() ? sim_clock::now() : impl->shared->lock()->wall_end;
    return end - start;
}

std::chrono::duration<double> osc::Fd_simulation::sim_current_time() const noexcept {
    return impl->shared->lock()->stats.time;
}

std::chrono::duration<double> osc::Fd_simulation::sim_final_time() const noexcept {
    return impl->final_time;
}

char const* osc::Fd_simulation::status_description() const noexcept {
    switch (impl->shared->lock()->status) {
    case Sim_status::Running:
        return "running";
    case Sim_status::Completed:
        return "completed";
    case Sim_status::Cancelled:
        return "cancelled";
    case Sim_status::Error:
        return "error";
    default:
        return "UNKNOWN STATUS: DEV ERROR";
    }
}

Simulation_stats osc::Fd_simulation::stats() const noexcept {
    return impl->shared->lock()->stats;
}

void osc::Fd_simulation::request_stop() noexcept {
    impl->simulator_thread.request_stop();
}

void osc::Fd_simulation::stop() noexcept {
    impl->simulator_thread.request_stop();
    impl->simulator_thread.join();
}
