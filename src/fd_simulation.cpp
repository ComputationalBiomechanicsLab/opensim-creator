#include "fd_simulation.hpp"

#include "opensim_wrapper.hpp"
#include "shims.hpp"

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

    Threadsafe_fdsim_state(osmv::State initial_state) :
        state{std::move(initial_state)} {
    }
};

// runs a forward dynamic simuation, updating `shared` (which is designed to
// be thread-safe) as it runs
static int run_fd_simulation(
        shims::stop_token stop_tok,
        osmv::Fd_simulation_params params,
        Threadsafe_fdsim_state& shared) {

    osmv::finalize_properties_from_state(params.model, shared.state.v);
    osmv::init_system(params.model);

    using clock = std::chrono::steady_clock;

    shared.wall_start = clock::now().time_since_epoch();
    shared.status = Sim_status::Running;

    clock::time_point last_report_end = clock::now();

    osmv::Fd_sim_config config;
    config.final_time = params.final_time;
    config.on_integration_step = [&shared, &stop_tok, &params, &last_report_end](SimTK::State const& s) {
        // if a stop was requested, throw an exception to interrupt the simulation
        //
        // this (hacky) approach is because the simulation is black-boxed at the moment - a better
        // solution would evaluate the token mid-simulation and interrupt it gracefully.
        if (stop_tok.stop_requested()) {
            shared.status = Sim_status::Cancelled;
            return osmv::Callback_response::Please_halt;
        }

        clock::time_point report_start = clock::now();

        shared.state.try_apply_a([&s](osmv::State& s_other) {
            s_other = s;
        });
        shared.num_pq_calls = osmv::num_prescribeq_calls(params.model);
        shared.sim_cur_time = osmv::simulation_time(s);

        clock::time_point report_end = clock::now();

        if (shared.ui_overhead_n > 0) {
            clock::duration ui_time = report_end - report_start;
            clock::duration total = report_end - last_report_end;
            double this_overhead = static_cast<double>(ui_time.count()) / static_cast<double>(total.count());
            shared.ui_overhead_acc = shared.ui_overhead_acc.load() +  this_overhead;
        }

        last_report_end = report_end;
        ++shared.ui_overhead_n;

        return osmv::Callback_response::Ok;
    };

    // run the simulation
    try {
        osmv::fd_simulation(params.model, std::move(params.initial_state), std::move(config));
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
            worker{run_fd_simulation, std::move(p), std::ref(shared)}
        {
        }
    };

    Fd_simulation::Fd_simulation(Fd_simulation_params p) :
        impl{new Fd_simulation_impl{std::move(p)}}
    {
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
        clock::duration endpoint =
            is_running() ? clock::now().time_since_epoch() : impl->shared.wall_end.load();

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
