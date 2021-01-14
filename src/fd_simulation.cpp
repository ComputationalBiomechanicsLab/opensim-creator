#include "fd_simulation.hpp"

#include "opensim_wrapper.hpp"
#include "shims.hpp"

#include <OpenSim.h>
#include <optional>

// status of an OpenSim simulation
enum class Sim_status {
    Running = 1,
    Completed = 2,
    Cancelled = 4,
    Error = 8,
};

static char const* str(Sim_status s) noexcept {
    switch (s) {
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

// share a value between exactly two threads such that thread A can access the
// value, followed by thread B, followed by A again
//
// effectively, just a slightly more robust abstraction over having a shared
// value + flag (/w mutex or atomics)
template<typename T>
struct Passed_parcel final {
    enum class State { a, b, locked };

    T v;
    std::atomic<State> st = State::a;

    Passed_parcel(T _v) : v{std::move(_v)} {
    }

    template<typename Mutator>
    bool try_apply_a(Mutator f) {
        State expected = State::a;
        if (st.compare_exchange_strong(expected, State::locked)) {
            f(v);
            st = State::b;
            return true;
        }
        return false;
    }

    template<typename Mutator>
    bool try_apply_b(Mutator f) {
        State expected = State::b;
        if (st.compare_exchange_strong(expected, State::locked)) {
            f(v);
            st = State::a;
            return true;
        }
        return false;
    }
};

// state that is shared between a running simulator thread and
struct Threadsafe_fdsim_state {
    using clock = std::chrono::high_resolution_clock;

    Passed_parcel<osmv::State> state;
    std::atomic<double> sim_cur_time = 0.0;
    std::atomic<double> ui_overhead_acc = 0.0;
    std::atomic<clock::duration> wall_start = clock::now().time_since_epoch();
    std::atomic<clock::duration> wall_end = clock::now().time_since_epoch();
    std::atomic<int> num_pq_calls = 0;
    std::atomic<int> ui_overhead_n = 0;
    std::atomic<Sim_status> status = Sim_status::Running;

    Threadsafe_fdsim_state(osmv::State initial_state) : state{std::move(initial_state)} {
    }
};

// response from integration step callback
enum class Callback_response {
    Ok,  // callback executed ok
    Please_halt  // callback wants the simulator to halt
};

// top-level configuration for a basic forward-dynamic sim
struct Fd_sim_config final {
    double final_time = 0.4;
    int max_steps = 20000;
    double min_step_size = 1.0e-8;
    double max_step_size = 1.0;
    double integrator_accuracy = 1.0e-5;
    std::optional<std::function<Callback_response(SimTK::State const&)>> on_integration_step = std::nullopt;
};

// analysis: per integration step
struct CustomAnalysis final : public OpenSim::Analysis {
    OpenSim::Manager& manager;
    std::function<Callback_response(SimTK::State const&)> on_integration_step;

    CustomAnalysis(OpenSim::Manager& _m, std::function<Callback_response(SimTK::State const&)> _f) :
        manager{_m},
        on_integration_step{std::move(_f)} {
    }

    int begin(SimTK::State const& s) override {
        if (on_integration_step(s) == Callback_response::Please_halt) {
            manager.halt();
        }
        return 0;
    }

    int step(SimTK::State const& s, int) override {
        if (on_integration_step(s) == Callback_response::Please_halt) {
            manager.halt();
        }
        return 0;
    }

    int end(SimTK::State const& s) override {
        if (on_integration_step(s) == Callback_response::Please_halt) {
            manager.halt();
        }
        return 0;
    }

    CustomAnalysis* clone() const override {
        return new CustomAnalysis{manager, on_integration_step};
    }

    std::string const& getConcreteClassName() const override {
        static std::string const name = "CustomAnalysis";
        return name;
    }
};

static osmv::State fd_simulation(OpenSim::Model& model, osmv::State initial_state, Fd_sim_config const& config) {
    OpenSim::Manager manager{model};

    if (config.on_integration_step) {
        model.addAnalysis(new CustomAnalysis{manager, *config.on_integration_step});
    }

    manager.setWriteToStorage(false);
    manager.setIntegratorInternalStepLimit(config.max_steps);
    manager.setIntegratorMaximumStepSize(config.max_step_size);
    manager.setIntegratorMinimumStepSize(config.min_step_size);
    manager.setIntegratorAccuracy(config.integrator_accuracy);

    model.getMultibodySystem().realize(initial_state, SimTK::Stage::Position);
    model.equilibrateMuscles(initial_state);

    manager.initialize(initial_state);
    return osmv::State{manager.integrate(config.final_time)};
}

osmv::State osmv::run_fd_simulation(OpenSim::Model& model) {
    return fd_simulation(model, osmv::State{model.initSystem()}, Fd_sim_config{});
}

// runs a forward dynamic simuation, updating `shared` (which is designed to
// be thread-safe) as it runs
static int
    _run_fd_simulation(shims::stop_token stop_tok, osmv::Fd_simulation_params params, Threadsafe_fdsim_state& shared) {

    params.model->setPropertiesFromState(shared.state.v);
    params.model->initSystem();

    using clock = std::chrono::steady_clock;

    shared.wall_start = clock::now().time_since_epoch();
    shared.status = Sim_status::Running;

    clock::time_point simulation_thread_started = clock::now();
    clock::time_point last_report_end = clock::now();

    Fd_sim_config config;
    config.final_time = params.final_time;
    config.on_integration_step =
        [&shared, &stop_tok, &params, &last_report_end, &simulation_thread_started](SimTK::State const& s) {
            // if a stop was requested, throw an exception to interrupt the simulation
            //
            // this (hacky) approach is because the simulation is black-boxed at the moment - a better
            // solution would evaluate the token mid-simulation and interrupt it gracefully.
            if (stop_tok.stop_requested()) {
                shared.status = Sim_status::Cancelled;
                return Callback_response::Please_halt;
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

            shared.state.try_apply_a([&s](osmv::State& s_other) { s_other = s; });

            shared.num_pq_calls = params.model->getSystem().getNumPrescribeQCalls();
            shared.sim_cur_time = s.getTime();

            clock::time_point report_end = clock::now();

            if (shared.ui_overhead_n > 0) {
                clock::duration ui_time = report_end - report_start;
                clock::duration total = report_end - last_report_end;
                double this_overhead = static_cast<double>(ui_time.count()) / static_cast<double>(total.count());
                shared.ui_overhead_acc = shared.ui_overhead_acc.load() + this_overhead;
            }

            last_report_end = report_end;
            ++shared.ui_overhead_n;

            return Callback_response::Ok;
        };

    // run the simulation
    try {
        fd_simulation(params.model, std::move(params.initial_state), std::move(config));
        shared.wall_end = clock::now().time_since_epoch();
        shared.status = Sim_status::Completed;
    } catch (...) {
        shared.wall_end = clock::now().time_since_epoch();
        shared.status = Sim_status::Error;
        throw;
    }

    return 0;
}

namespace osmv {
    using clock = Fd_simulation::clock;

    struct Fd_simulation_impl final {
        double final_time = -1337.0;
        int states_popped = 0;

        // state that can be safely shared between the simulator thread and
        // the UI thread
        Threadsafe_fdsim_state shared;

        // the thread that's actually running OpenSim
        shims::jthread worker;

        Fd_simulation_impl(Fd_simulation_params p) :
            final_time{p.final_time},

            // copy the state into the shared passed-parcel (implementation detail)
            shared{State{p.initial_state}},

            // start the simulation
            worker{_run_fd_simulation, std::move(p), std::ref(shared)} {
        }
    };

    Fd_simulation::Fd_simulation(Fd_simulation_params p) : impl{new Fd_simulation_impl{std::move(p)}} {
    }

    Fd_simulation::~Fd_simulation() noexcept = default;

    bool Fd_simulation::try_pop_latest_state(osmv::State& dest) {
        return impl->shared.state.try_apply_b([&](osmv::State& latest) {
            std::swap(dest, latest);
            ++impl->states_popped;
        });
    }

    void Fd_simulation::request_stop() {
        impl->worker.request_stop();
    }

    bool Fd_simulation::is_running() const {
        return impl->shared.status == Sim_status::Running;
    }

    clock::duration Fd_simulation::wall_duration() const {
        clock::duration endpoint = is_running() ? clock::now().time_since_epoch() : impl->shared.wall_end.load();

        return endpoint - impl->shared.wall_start.load();
    }

    double Fd_simulation::sim_current_time() const {
        return impl->shared.sim_cur_time;
    }

    double Fd_simulation::sim_final_time() const {
        return impl->final_time;
    }

    char const* Fd_simulation::status_description() const {
        return str(impl->shared.status);
    }

    int Fd_simulation::num_prescribeq_calls() const {
        return impl->shared.num_pq_calls;
    }

    double Fd_simulation::avg_ui_overhead() const {
        return impl->shared.ui_overhead_acc / impl->shared.ui_overhead_n;
    }

    int Fd_simulation::num_states_popped() const {
        return impl->states_popped;
    }
}
