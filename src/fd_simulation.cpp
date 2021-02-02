#include "fd_simulation.hpp"

#include "opensim_wrapper.hpp"
#include "shims.hpp"

#include <OpenSim/Common/ComponentOutput.h>
#include <OpenSim/Simulation/Manager/Manager.h>
#include <OpenSim/Simulation/Model/Analysis.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <SimTKsimbody.h>
#include <simmath/Integrator.h>

#include <functional>
#include <mutex>
#include <ratio>
#include <stdexcept>
#include <string>
#include <thread>

namespace {
    using clock = std::chrono::steady_clock;

    // status of an OpenSim simulation
    enum class Sim_status {
        Running = 1,
        Completed = 2,
        Cancelled = 4,
        Error = 8,
    };

    // a mutex guard over a reference to `T`
    template<typename T>
    class Mutex_guard final {
        std::lock_guard<std::mutex> guard;
        T& ref;

    public:
        explicit Mutex_guard(std::mutex& mutex, T& _ref) : guard{mutex}, ref{_ref} {
        }

        T& operator*() noexcept {
            return ref;
        }

        T const& operator*() const noexcept {
            return ref;
        }

        T* operator->() noexcept {
            return &ref;
        }

        T const* operator->() const noexcept {
            return &ref;
        }
    };

    // represents a `T` value that can only be accessed via a mutex guard
    template<typename T>
    class Mutexed {
        std::mutex mutex;
        T value;

    public:
        explicit Mutexed(T _value) : value{std::move(_value)} {
        }

        // in-place constructor for T
        template<typename... Args>
        explicit Mutexed(Args... args) : value{std::forward<Args...>(args)...} {
        }

        Mutex_guard<T> lock() {
            return Mutex_guard<T>{mutex, value};
        }
    };

    // state that is shared between the simulator owner (i.e. the UI thread) and the simulation
    // thread
    //
    // the contract here is that the simulator thread will try to update these values often so that
    // the owner can monitor simulation progress.
    struct Shared_fdsim_state final {

        // the simulator thread will *copy* its latest state into here if it sees that it is
        // a nullptr
        //
        // this means that the UI thread can occasionally poll for the latest state by exchanging
        // it out for a nullptr (which will make the simulator thread write a new update on the
        // next go-round)
        std::unique_ptr<SimTK::State> latest_state = nullptr;

        double sim_cur_time = 0.0;
        double ui_overhead_acc = 0.0;
        clock::duration wall_start = clock::now().time_since_epoch();
        clock::duration wall_end = clock::now().time_since_epoch();
        int num_prescribeq_calls = 0;
        int ui_overhead_n = 0;
        osmv::Integrator_stats istats;
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
        osmv::stop_token stop_token,
        osmv::Fd_simulation_params params,
        std::shared_ptr<Mutexed<Shared_fdsim_state>> thread_shared) {

        // setup model + manager
        params.model->setPropertiesFromState(params.initial_state);
        params.model->initSystem();
        OpenSim::Manager manager{params.model};

        // select integrator
        {
            switch (params.integrator_method) {
            case osmv::IntegratorMethod_OpenSimManagerDefault:
                // just use whatever Manager is already using
                break;
            case osmv::IntegratorMethod_ExplicitEuler:
                manager.setIntegratorMethod(OpenSim::Manager::IntegratorMethod::ExplicitEuler);
                break;
            case osmv::IntegratorMethod_RungeKutta2:
                manager.setIntegratorMethod(OpenSim::Manager::IntegratorMethod::RungeKutta2);
                break;
            case osmv::IntegratorMethod_RungeKutta3:
                manager.setIntegratorMethod(OpenSim::Manager::IntegratorMethod::RungeKutta3);
                break;
            case osmv::IntegratorMethod_RungeKuttaFeldberg:
                manager.setIntegratorMethod(OpenSim::Manager::IntegratorMethod::RungeKuttaFeldberg);
                break;
            case osmv::IntegratorMethod_RungeKuttaMerson:
                manager.setIntegratorMethod(OpenSim::Manager::IntegratorMethod::RungeKuttaMerson);
                break;
            case osmv::IntegratorMethod_SemiExplicitEuler2:
                manager.setIntegratorMethod(OpenSim::Manager::IntegratorMethod::SemiExplicitEuler2);
                break;
            case osmv::IntegratorMethod_Verlet:
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
        int steps = 0;

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
                std::chrono::microseconds wall_time{
                    std::chrono::duration_cast<std::chrono::microseconds>(report_start - simulation_thread_started)};

                if (sim_time > wall_time) {
                    std::this_thread::sleep_for(sim_time - wall_time);
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

            // other reporting: copy relevant values out to threadsafe location

            shared_st.sim_cur_time = s.getTime();
            shared_st.num_prescribeq_calls = params.model->getSystem().getNumPrescribeQCalls();

            // assign stats
            {
                SimTK::Integrator const& integrator = manager.getIntegrator();

                osmv::Integrator_stats& is = shared_st.istats;
                is.accuracyInUse = static_cast<float>(integrator.getAccuracyInUse());
                is.predictedNextStepSize = static_cast<float>(integrator.getPredictedNextStepSize());
                is.numStepsAttempted = integrator.getNumStepsAttempted();
                is.numStepsTaken = integrator.getNumStepsTaken();
                is.numRealizations = integrator.getNumRealizations();
                is.numQProjections = integrator.getNumQProjections();
                is.numUProjections = integrator.getNumUProjections();
                is.numErrorTestFailures = integrator.getNumErrorTestFailures();
                is.numConvergenceTestFailures = integrator.getNumConvergenceTestFailures();
                is.numRealizationFailures = integrator.getNumRealizationFailures();
                is.numQProjectionFailures = integrator.getNumQProjectionFailures();
                is.numUProjectionFailures = integrator.getNumUProjectionFailures();
                is.numProjectionFailures = integrator.getNumProjectionFailures();
                is.numConvergentIterations = integrator.getNumConvergentIterations();
                is.numDivergentIterations = integrator.getNumDivergentIterations();
                is.numIterations = integrator.getNumIterations();
            }

            if (steps++ > 0) {
                clock::duration total = report_start - last_report_start;
                clock::duration overhead = last_report_end - last_report_start;
                double overhead_fraction = static_cast<double>(overhead.count()) / static_cast<double>(total.count());
                shared_st.ui_overhead_acc = shared_st.ui_overhead_acc + overhead_fraction;
                ++shared_st.ui_overhead_n;
            }

            // loop invariants
            last_report_start = report_start;
            last_report_end = clock::now();
        }));

        // finish configuring manager + model etc.
        config_manager_with_params_similar_to_forwardtool(manager);
        params.model->getMultibodySystem().realize(params.initial_state, SimTK::Stage::Position);
        params.model->equilibrateMuscles(params.initial_state);

        // start the simulation
        try {
            manager.initialize(params.initial_state);
            manager.integrate(params.final_time);

            auto guard = thread_shared->lock();
            guard->wall_end = clock::now().time_since_epoch();
            guard->status = Sim_status::Completed;
        } catch (...) {
            auto guard = thread_shared->lock();
            guard->wall_end = clock::now().time_since_epoch();
            guard->status = Sim_status::Error;
            throw;
        }

        return 0;
    }
}

namespace osmv {
    struct Fd_simulator_impl final {
        double final_time = -1337.0;
        int states_popped = 0;

        // state that is accessible by both the main (probably, UI) thread
        // and the background simulator thread
        std::shared_ptr<Mutexed<Shared_fdsim_state>> shared = std::make_shared<Mutexed<Shared_fdsim_state>>();

        // the simulator thread. Jthreads automatically send a cancellation request
        // and join on destruction, so destroying a simulator *should* automatically
        // cancel + wait
        jthread simulator_thread;

        Fd_simulator_impl(Fd_simulation_params p) :
            final_time{p.final_time},

            // start the simulation
            simulator_thread{simulation_thread_main, std::move(p), shared} {
        }
    };
}

osmv::IntegratorMethod const osmv::integrator_methods[IntegratorMethod_NumIntegratorMethods] = {
    IntegratorMethod_OpenSimManagerDefault,
    IntegratorMethod_ExplicitEuler,
    IntegratorMethod_RungeKutta2,
    IntegratorMethod_RungeKutta3,
    IntegratorMethod_RungeKuttaFeldberg,
    IntegratorMethod_RungeKuttaMerson,
    IntegratorMethod_SemiExplicitEuler2,
    IntegratorMethod_Verlet,
};

char const* const osmv::integrator_method_names[IntegratorMethod_NumIntegratorMethods] = {
    "OpenSim::Manager Default",
    "Explicit Euler",
    "Runge Kutta 2",
    "Runge Kutta 3",
    "Runge Kutta Feldberg",
    "Runge Kutta Merson",
    "Semi Explicit Euler 2",
    "Verlet",
};

osmv::Fd_simulator::Fd_simulator(Fd_simulation_params p) : impl{new Fd_simulator_impl{std::move(p)}} {
}

osmv::Fd_simulator::~Fd_simulator() noexcept = default;

bool osmv::Fd_simulator::try_pop_state(osmv::State& dest) {
    std::unique_ptr<SimTK::State> maybe_latest = std::move(impl->shared->lock()->latest_state);
    if (maybe_latest) {
        dest = std::move(maybe_latest);

        ++impl->states_popped;
        return true;
    }

    return false;
}

void osmv::Fd_simulator::request_stop() {
    impl->simulator_thread.request_stop();
}

void osmv::Fd_simulator::stop() {
    impl->simulator_thread.request_stop();
    impl->simulator_thread.join();
}

bool osmv::Fd_simulator::is_running() const {
    return impl->shared->lock()->status == Sim_status::Running;
}

std::chrono::duration<double> osmv::Fd_simulator::wall_duration() const {
    clock::duration endpoint = is_running() ? clock::now().time_since_epoch() : impl->shared->lock()->wall_end;

    return endpoint - impl->shared->lock()->wall_start;
}

std::chrono::duration<double> osmv::Fd_simulator::sim_current_time() const {
    return std::chrono::duration<double>{impl->shared->lock()->sim_cur_time};
}

std::chrono::duration<double> osmv::Fd_simulator::sim_final_time() const {
    return std::chrono::duration<double>{impl->final_time};
}

char const* osmv::Fd_simulator::status_description() const {
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

int osmv::Fd_simulator::num_prescribeq_calls() const {
    return impl->shared->lock()->num_prescribeq_calls;
}

void osmv::Fd_simulator::integrator_stats(Integrator_stats& out) const noexcept {
    auto guard = impl->shared->lock();
    out = guard->istats;
}

double osmv::Fd_simulator::avg_simulator_overhead() const {
    auto guard = impl->shared->lock();
    return guard->ui_overhead_acc / guard->ui_overhead_n;
}

int osmv::Fd_simulator::num_states_popped() const {
    return impl->states_popped;
}

// other method implementations

osmv::State osmv::run_fd_simulation(OpenSim::Model& model) {
    SimTK::State initial_state{model.initSystem()};
    OpenSim::Manager manager{model};

    config_manager_with_params_similar_to_forwardtool(manager);

    model.getMultibodySystem().realize(initial_state, SimTK::Stage::Position);
    model.equilibrateMuscles(initial_state);

    manager.initialize(initial_state);

    return osmv::State{manager.integrate(0.4)};
}
