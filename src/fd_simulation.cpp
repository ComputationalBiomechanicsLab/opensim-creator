#include "fd_simulation.hpp"

#include "opensim_wrapper.hpp"
#include "shims.hpp"

#include <OpenSim.h>

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
        int num_pq_calls = 0;
        int ui_overhead_n = 0;
        int num_integration_steps = 0;
        int num_integration_step_attempts = 0;
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
        shims::stop_token stop_token,
        osmv::Fd_simulation_params params,
        std::shared_ptr<Mutexed<Shared_fdsim_state>> thread_shared) {

        clock::time_point simulation_thread_started = clock::now();
        clock::time_point last_report_end = clock::now();

        params.model->setPropertiesFromState(params.initial_state);
        params.model->initSystem();
        OpenSim::Manager manager{params.model};

        // add an analysis that calls on each integration step
        params.model->addAnalysis(new Lambda_analysis([&](SimTK::State const& s) {
            // propagate stop_token signal to the OpenSim::Manager running this sim, which enables
            // cancellation behavior
            if (stop_token.stop_requested()) {
                manager.halt();
            }

            clock::time_point report_start = clock::now();

            if (params.throttle_to_wall_time) {
                // caller requests that the simulation thread tries to run roughly as
                // fast as wall time
                //
                // this is important in very cheap simulations that run *much* faster
                // than wall time, where the user won't be able to see each state
                double sim_secs = s.getTime();
                std::chrono::microseconds sim_micros{static_cast<long>(1000000.0 * sim_secs)};
                std::chrono::microseconds real_micros =
                    std::chrono::duration_cast<std::chrono::microseconds>(report_start - simulation_thread_started);
                if (sim_micros > real_micros) {
                    std::this_thread::sleep_for(sim_micros - real_micros);
                }
            }

            auto mutex_guard = thread_shared->lock();
            Shared_fdsim_state& shared_st = *mutex_guard;

            if (shared_st.latest_state == nullptr) {
                // the UI thread probably "stole" the latest state, so copy the new (later) one
                // so that the UI thread may take it
                shared_st.latest_state = std::make_unique<SimTK::State>(s);
            }

            shared_st.num_pq_calls = params.model->getSystem().getNumPrescribeQCalls();
            shared_st.num_integration_steps++;
            shared_st.num_integration_step_attempts = manager.getIntegrator().getNumStepsAttempted();
            shared_st.sim_cur_time = s.getTime();

            clock::time_point report_end = clock::now();

            if (shared_st.ui_overhead_n > 0) {
                clock::duration ui_time = report_end - report_start;
                clock::duration total = report_end - last_report_end;
                double this_overhead = static_cast<double>(ui_time.count()) / static_cast<double>(total.count());
                shared_st.ui_overhead_acc = shared_st.ui_overhead_acc + this_overhead;
            }

            last_report_end = report_end;
            shared_st.ui_overhead_n++;
        }));

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
        shims::jthread simulator_thread;

        Fd_simulator_impl(Fd_simulation_params p) :
            final_time{p.final_time},

            // start the simulation
            simulator_thread{simulation_thread_main, std::move(p), shared} {
        }
    };
}

// osmv::Fd_simulator interface implementation

osmv::Fd_simulator::Fd_simulator(Fd_simulation_params p) : impl{new Fd_simulator_impl{std::move(p)}} {
}

osmv::Fd_simulator::~Fd_simulator() noexcept = default;

bool osmv::Fd_simulator::try_pop_latest_state(osmv::State& dest) {
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
    return impl->shared->lock()->num_pq_calls;
}

int osmv::Fd_simulator::num_integration_steps() const {
    return impl->shared->lock()->num_integration_steps;
}

int osmv::Fd_simulator::num_integration_step_attempts() const {
    return impl->shared->lock()->num_integration_step_attempts;
}

double osmv::Fd_simulator::avg_ui_overhead_pct() const {
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
