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

namespace {
    using clock = std::chrono::steady_clock;

    // status of an OpenSim simulation
    enum class Sim_status {
        Running = 1,
        Completed = 2,
        Cancelled = 4,
        Error = 8,
    };

    // state that is shared between the simulator owner (i.e. the UI thread) and the simulation
    // thread
    //
    // the contract here is that the simulator thread will try to update these values often so that
    // the owner can monitor simulation progress.
    struct Shared_fdsim_state final {
        // this will only be updated by the sim thread when it is nullptr
        //
        // a successful Fd_simulation::try_pop_state will make this into a nullptr
        std::unique_ptr<SimTK::State> latest_state = nullptr;

        clock::time_point wall_start = clock::now();
        clock::time_point wall_end = clock::now();
        Simulation_stats stats{};
        Sim_status status = Sim_status::Running;
    };

    // an OpenSim::Analysis that calls an arbitrary callback function with the latest SimTK::State
    template<typename Callback>
    struct Lambda_analysis final : public OpenSim::Analysis {
        Callback callback;

        Lambda_analysis(Callback _callback) : callback{std::move(_callback)} {
        }

        int begin(SimTK::State const& s) override {
            callback(std::ref(s));
            return 0;
        }

        int step(SimTK::State const& s, int) override {
            callback(std::ref(s));
            return 0;
        }

        int end(SimTK::State const& s) override {
            callback(std::ref(s));
            return 0;
        }

        Lambda_analysis* clone() const override {
            return new Lambda_analysis{callback};
        }

        std::string const& getConcreteClassName() const override {
            static std::string const name = "LambdaAnalysis";
            return name;
        }
    };

    void config_manager_with_params_similar_to_forwardtool(OpenSim::Manager& manager) {
        manager.setWriteToStorage(false);
        manager.setIntegratorInternalStepLimit(20000);
        manager.setIntegratorMinimumStepSize(1.0e-8);
        manager.setIntegratorMaximumStepSize(1.0);
        manager.setIntegratorAccuracy(1.0e-5);
    }

    // MAIN: simulator thread: this is the top-level function that the simulator thread executes
    // in the background.
    int simulation_thread_main(
        stop_token stop_token,
        Fd_simulation_params params,
        std::shared_ptr<Mutex_guarded<Shared_fdsim_state>> thread_shared) {

        // setup model + manager
        params.model->setPropertiesFromState(*params.state);
        params.model->initSystem();
        OpenSim::Manager manager{*params.model};

        // select integrator
        {
            switch (params.integrator_method) {
            case osc::IntegratorMethod_OpenSimManagerDefault:
                // just use whatever Manager is already using
                break;
            case osc::IntegratorMethod_ExplicitEuler:
                manager.setIntegratorMethod(OpenSim::Manager::IntegratorMethod::ExplicitEuler);
                break;
            case osc::IntegratorMethod_RungeKutta2:
                manager.setIntegratorMethod(OpenSim::Manager::IntegratorMethod::RungeKutta2);
                break;
            case osc::IntegratorMethod_RungeKutta3:
                manager.setIntegratorMethod(OpenSim::Manager::IntegratorMethod::RungeKutta3);
                break;
            case osc::IntegratorMethod_RungeKuttaFeldberg:
                manager.setIntegratorMethod(OpenSim::Manager::IntegratorMethod::RungeKuttaFeldberg);
                break;
            case osc::IntegratorMethod_RungeKuttaMerson:
                manager.setIntegratorMethod(OpenSim::Manager::IntegratorMethod::RungeKuttaMerson);
                break;
            case osc::IntegratorMethod_SemiExplicitEuler2:
                manager.setIntegratorMethod(OpenSim::Manager::IntegratorMethod::SemiExplicitEuler2);
                break;
            case osc::IntegratorMethod_Verlet:
                manager.setIntegratorMethod(OpenSim::Manager::IntegratorMethod::Verlet);
                break;
            default:
                throw std::runtime_error{"unknown integrator method passed to simulation_thread_main"};
            }
        }

        // add an analysis that calls on each integration step
        clock::time_point simulation_thread_started = clock::now();
        clock::time_point last_report_start = clock::now();
        clock::time_point last_report_end = clock::now();
        std::chrono::microseconds sim_initial_time{static_cast<long>(1000000.0 * params.state->getTime())};

        params.model->addAnalysis(new Lambda_analysis([&](SimTK::State const& s) {
            // simulation cancellation
            //
            // if the stop token (inter-thread cancellation request token) is set, halt the
            // simulation
            if (stop_token.stop_requested()) {
                manager.halt();
            }

            // meta: report how long it's taking to perform these per-integration-step steps
            clock::time_point report_start = clock::now();

            // throttling: slow down the simulation thread if requested
            //
            // if the simulation is running faster than wall time and the caller requested that
            // it runs slower than wall time then sleep the simulation thread
            if (params.throttle_to_wall_time) {
                std::chrono::microseconds sim_time{static_cast<long>(1000000.0 * s.getTime())};
                std::chrono::microseconds sim_elapsed = sim_time - sim_initial_time;
                std::chrono::microseconds wall_elapsed{
                    std::chrono::duration_cast<std::chrono::microseconds>(report_start - simulation_thread_started)};

                if (sim_elapsed > wall_elapsed) {
                    std::this_thread::sleep_for(sim_elapsed - wall_elapsed);
                }
            }

            // inter-thread reporting
            //
            // copy any relevant information out in a threadsafe way

            Mutex_guard<Shared_fdsim_state> guard{thread_shared->lock()};
            Shared_fdsim_state& shared_st = *guard;

            // state reporting
            //
            // only copy a state if the "message space" is empty
            //
            // this is because copying a state is potentially quite expensive, so should only be
            // performed when necessary
            if (shared_st.latest_state == nullptr) {
                shared_st.latest_state = std::make_unique<SimTK::State>(s);
            }

            // assign stats
            {
                SimTK::Integrator const& integrator = manager.getIntegrator();
                auto& stats = shared_st.stats;

                stats.time = std::chrono::duration<double>{s.getTime()};

                {
                    clock::duration total = report_start - last_report_start;
                    clock::duration overhead = last_report_end - last_report_start;
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

            // loop invariants
            last_report_start = report_start;
            last_report_end = clock::now();
        }));

        // finish configuring manager + model etc.
        config_manager_with_params_similar_to_forwardtool(manager);
        params.model->getMultibodySystem().realize(*params.state, SimTK::Stage::Position);
        params.model->equilibrateMuscles(*params.state);

        // start the simulation
        try {
            manager.initialize(*params.state);
            manager.integrate(params.final_time.count());

            auto guard = thread_shared->lock();
            guard->wall_end = clock::now();
            guard->status = Sim_status::Completed;
        } catch (...) {
            auto guard = thread_shared->lock();
            guard->wall_end = clock::now();
            guard->status = Sim_status::Error;
            throw;
        }

        return 0;
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
    std::chrono::duration<double> final_time = std::chrono::duration<double>(-1337.0);

    // state that is accessible by both the main (probably, UI) thread
    // and the background simulator thread
    std::shared_ptr<Mutex_guarded<Shared_fdsim_state>> shared = std::make_shared<Mutex_guarded<Shared_fdsim_state>>();

    // the simulator thread. Jthreads automatically send a cancellation request
    // and join on destruction, so destroying a simulator *should* automatically
    // cancel + wait
    jthread simulator_thread;

    int states_popped = 0;

    Impl(Fd_simulation_params p) :
        final_time{p.final_time},

        // start the simulation
        simulator_thread{simulation_thread_main, std::move(p), shared} {
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
    clock::time_point start = impl->shared->lock()->wall_start;
    clock::time_point end = is_running() ? clock::now() : impl->shared->lock()->wall_end;
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
