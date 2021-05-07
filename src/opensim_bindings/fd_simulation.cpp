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
using namespace osc::fd;
using sim_clock = std::chrono::steady_clock;

enum class Fdsim_status {
    Running = 1,
    Completed,
    Cancelled,
    Error,
};

static SimTK::Integrator* fdsim_make_integator_unguarded(SimTK::System const& system, IntegratorMethod method) {
    switch (method) {
    case IntegratorMethod_OpenSimManagerDefault:
        return new SimTK::RungeKuttaMersonIntegrator{system};
    case IntegratorMethod_ExplicitEuler:
        return new SimTK::ExplicitEulerIntegrator{system};
    case IntegratorMethod_RungeKutta2:
        return new SimTK::RungeKutta2Integrator{system};
    case IntegratorMethod_RungeKutta3:
        return new SimTK::RungeKutta3Integrator{system};
    case IntegratorMethod_RungeKuttaFeldberg:
        return new SimTK::RungeKuttaFeldbergIntegrator{system};
    case IntegratorMethod_RungeKuttaMerson:
        return new SimTK::RungeKuttaMersonIntegrator{system};
    case IntegratorMethod_SemiExplicitEuler2:
        return new SimTK::SemiExplicitEuler2Integrator{system};
    case IntegratorMethod_Verlet:
        return new SimTK::VerletIntegrator{system};
    default:
        return new SimTK::RungeKuttaMersonIntegrator{system};
    }
}

static std::unique_ptr<SimTK::Integrator> fdsim_make_integrator(SimTK::System const& system, IntegratorMethod method) {
    return std::unique_ptr<SimTK::Integrator>{fdsim_make_integator_unguarded(system, method)};
}

// state that is shared between UI and simulation thread
struct Shared_state final {
    Fdsim_status status = Fdsim_status::Running;

    sim_clock::time_point wall_start = sim_clock::now();
    sim_clock::time_point wall_end = sim_clock::now();  // only updated at the end
    std::chrono::duration<double> latest_sim_time{0};

    std::unique_ptr<Report> latest_report = nullptr;
    std::vector<std::unique_ptr<Report>> regular_reports;
};

static std::unique_ptr<Report> fdsim_make_report(Params const& params, SimTK::Integrator const& integrator) {
    std::unique_ptr<Report> out = std::make_unique<Report>();

    out->state = integrator.getState();

    Stats& stats = out->stats;

    // integrator stats
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

    // system stats
    stats.numPrescribeQcalls =  params.model->getSystem().getNumPrescribeQCalls();

    return out;
}

[[nodiscard]] static bool eq(double a, double b) noexcept {
    // why:
    //
    //     http://realtimecollisiondetection.net/blog/?p=89
    //     https://stackoverflow.com/questions/17333/what-is-the-most-effective-way-for-float-and-double-comparison
    //
    // effectively, epsilon is "machine epsilon", and is only relevant for
    // numbers < 1.0. It has to be scaled up to the magnitude of the operands

    double scaled_epsilon =
        std::max(1.0, std::max(std::abs(a), std::abs(b))) * std::numeric_limits<double>::epsilon();

    return std::abs(a - b) < scaled_epsilon;
}

static Fdsim_status fdsim_main_unguarded(stop_token stop_token,
                                         Params params,
                                         std::shared_ptr<Mutex_guarded<Shared_state>> shared) {

    params.model->setPropertiesFromState(*params.state);
    SimTK::State& st = params.model->initSystem();
    params.model->realizePosition(st);
    params.model->equilibrateMuscles(st);
    params.model->realizeAcceleration(st);

    std::unique_ptr<SimTK::Integrator> integ =
        fdsim_make_integrator(params.model->getMultibodySystem(), params.integrator_method);
    integ->setInternalStepLimit(params.integrator_step_limit);
    integ->setMinimumStepSize(params.integrator_minimum_step_size.count());
    integ->setMaximumStepSize(params.integrator_maximum_step_size.count());
    integ->setAccuracy(params.integrator_accuracy);
    integ->setFinalTime(params.final_time.count());
    integ->setReturnEveryInternalStep(params.update_latest_state_on_every_step);
    integ->initialize(st);

    SimTK::TimeStepper ts{params.model->getMultibodySystem(), *integ};
    ts.initialize(integ->getState());
    ts.setReportAllSignificantStates(params.update_latest_state_on_every_step);

    double t0 = integ->getTime();
    auto t0_wall = sim_clock::now();
    double tfinal = params.final_time.count();
    double tnext_regular_report = t0;

    // report t0
    {
        auto regular_report = fdsim_make_report(params, *integ);
        std::unique_ptr<Report> spot_report = nullptr;

        if (shared->lock()->latest_report == nullptr) {
            spot_report = std::make_unique<Report>(*regular_report);
        }

        auto guard = shared->lock();
        guard->regular_reports.push_back(std::move(regular_report));
        guard->latest_report = std::move(spot_report);
        tnext_regular_report = t0 + params.reporting_interval.count();
    }

    // integrate (t0..tfinal]
    for (double t = t0; t < tfinal; t = integ->getTime()) {

        // handle cancellation requests
        if (stop_token.stop_requested()) {
            return Fdsim_status::Cancelled;
        }

        // handle CPU throttling
        if (params.throttle_to_wall_time) {
            double dt = t - t0;
            std::chrono::microseconds dt_sim_micros =
                std::chrono::microseconds{static_cast<long>(1000000.0 * dt)};

            auto t_wall = sim_clock::now();
            auto dt_wall = t_wall - t0_wall;
            auto dt_wall_micros =
                std::chrono::duration_cast<std::chrono::microseconds>(dt_wall);

            if (dt_sim_micros > dt_wall_micros) {
                std::this_thread::sleep_for(dt_sim_micros - dt_wall_micros);
            }
        }

        // compute an integration step
        auto timestep_rv = ts.stepTo(std::min(tnext_regular_report, tfinal));

        // handle integration errors
        if (integ->isSimulationOver() &&
            integ->getTerminationReason() != SimTK::Integrator::ReachedFinalTime) {

            std::string reason = integ->getTerminationReasonString(integ->getTerminationReason());
            log::error("simulation error: integration failed: %s", reason.c_str());
            return Fdsim_status::Error;
        }

        // skip uninteresting integration steps
        if (timestep_rv != SimTK::Integrator::TimeHasAdvanced &&
            timestep_rv != SimTK::Integrator::ReachedScheduledEvent &&
            timestep_rv != SimTK::Integrator::ReachedReportTime &&
            timestep_rv != SimTK::Integrator::ReachedStepLimit) {

            continue;
        }

        // report integration step
        {
            std::unique_ptr<Report> regular_report = nullptr;
            std::unique_ptr<Report> spot_report = nullptr;

            // create regular report (if necessary)
            if (eq(tnext_regular_report, integ->getTime())) {
                regular_report = fdsim_make_report(params, *integ);
                tnext_regular_report = integ->getTime() + params.reporting_interval.count();
            }

            // create spot report (if necessary)
            if (shared->lock()->latest_report == nullptr) {
                if (regular_report) {
                    // just copy the already-created regular report
                    spot_report = std::make_unique<Report>(*regular_report);
                } else {
                    // make new report
                    spot_report = fdsim_make_report(params, *integ);
                }
            }

            // throw the reports over the fence to the calling thread
            auto guard = shared->lock();
            guard->latest_sim_time = std::chrono::duration<double>{integ->getTime()};
            if (regular_report) {
                guard->regular_reports.push_back(std::move(regular_report));
            }
            if (spot_report) {
                guard->latest_report = std::move(spot_report);
            }
        }
    }

    return Fdsim_status::Completed;
}

// MAIN: simulator thread: this is the top-level function that the simulator thread
// executes in the background.
static int fdsim_main(
    stop_token stop_token,
    Params params,
    std::shared_ptr<Mutex_guarded<Shared_state>> shared) {

    Fdsim_status status = Fdsim_status::Error;

    try {
        status = fdsim_main_unguarded(std::move(stop_token), std::move(params), shared);
    } catch (OpenSim::Exception const& ex) {
        log::error("OpenSim::Exception occurred when running a simulation: %s", ex.what());
    } catch (std::exception const& ex) {
        log::error("std::exception occurred when running a simulation: %s", ex.what());
    } catch (...) {
        log::error("an exception with unknown type occurred when running a simulation (no error message available)");
    }

    auto guard = shared->lock();
    guard->wall_end = sim_clock::now();
    guard->status = status;

    return 0;
}

IntegratorMethod const osc::fd::integrator_methods[IntegratorMethod_NumIntegratorMethods] = {
    IntegratorMethod_OpenSimManagerDefault,
    IntegratorMethod_ExplicitEuler,
    IntegratorMethod_RungeKutta2,
    IntegratorMethod_RungeKutta3,
    IntegratorMethod_RungeKuttaFeldberg,
    IntegratorMethod_RungeKuttaMerson,
    IntegratorMethod_SemiExplicitEuler2,
    IntegratorMethod_Verlet,
};

char const* const osc::fd::integrator_method_names[IntegratorMethod_NumIntegratorMethods] = {
    "OpenSim::Manager Default",
    "Explicit Euler",
    "Runge Kutta 2",
    "Runge Kutta 3",
    "Runge Kutta Feldberg",
    "Runge Kutta Merson",
    "Semi Explicit Euler 2",
    "Verlet",
};

struct osc::fd::Simulation::Impl final {
    std::chrono::duration<double> final_time;
    std::shared_ptr<Mutex_guarded<Shared_state>> shared;
    jthread simulator_thread;
    int states_popped;

    Impl(Params p) :
        final_time{p.final_time},

        shared{new Mutex_guarded<Shared_state>{}},

        // starts the simulation
        simulator_thread{fdsim_main, std::move(p), shared},

        states_popped{0} {
    }
};

osc::fd::Simulation::Simulation(Params p) : impl{new Impl{std::move(p)}} {
}

osc::fd::Simulation::~Simulation() noexcept = default;

std::unique_ptr<Report> osc::fd::Simulation::try_pop_latest_report() {
    std::unique_ptr<Report> rv = nullptr;

    {
        auto guard = impl->shared->lock();
        rv = std::move(guard->latest_report);
    }

    if (rv) {
        ++impl->states_popped;
    }

    return rv;
}

int osc::fd::Simulation::num_latest_reports_popped() const noexcept {
    return impl->states_popped;
}

bool osc::fd::Simulation::is_running() const noexcept {
    auto guard = impl->shared->lock();
    return guard->status == Fdsim_status::Running;
}

std::chrono::duration<double> osc::fd::Simulation::wall_duration() const noexcept {
    auto guard = impl->shared->lock();
    auto end = guard->status == Fdsim_status::Running ? sim_clock::now() : guard->wall_end;
    return end - guard->wall_start;
}

std::chrono::duration<double> osc::fd::Simulation::sim_current_time() const noexcept {
    auto guard = impl->shared->lock();
    return guard->latest_sim_time;
}

std::chrono::duration<double> osc::fd::Simulation::sim_final_time() const noexcept {
    return impl->final_time;
}

char const* osc::fd::Simulation::status_description() const noexcept {
    switch (impl->shared->lock()->status) {
    case Fdsim_status::Running:
        return "running";
    case Fdsim_status::Completed:
        return "completed";
    case Fdsim_status::Cancelled:
        return "cancelled";
    case Fdsim_status::Error:
        return "error";
    default:
        return "UNKNOWN STATUS: DEV ERROR";
    }
}

void osc::fd::Simulation::request_stop() noexcept {
    impl->simulator_thread.request_stop();
}

void osc::fd::Simulation::stop() noexcept {
    impl->simulator_thread.request_stop();
    impl->simulator_thread.join();
}
