#pragma once

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
int run_fd_simulation(
        shims::stop_token stop_tok,
        osmv::Model model,
        osmv::State initial_state,
        double final_time,
        Threadsafe_fdsim_state& shared) {

    class Stopped_exception final {};  // thrown to interrupt the simulation

    using clock = std::chrono::steady_clock;

    shared.wall_start = clock::now().time_since_epoch();
    shared.status = Sim_status::Running;

    clock::time_point last_report_end = clock::now();

    osmv::Fd_sim_config config;
    config.final_time = final_time;
    config.on_integration_step = [&shared, &stop_tok, &model, &last_report_end](SimTK::State const& s) {
        // if a stop was requested, throw an exception to interrupt the simulation
        //
        // this (hacky) approach is because the simulation is black-boxed at the moment - a better
        // solution would evaluate the token mid-simulation and interrupt it gracefully.
        if (stop_tok.stop_requested()) {
            throw Stopped_exception{};
        }

        clock::time_point report_start = clock::now();

        shared.state.try_apply_a([&s](osmv::State& s_other) {
            s_other = s;
        });
        shared.num_pq_calls = osmv::num_prescribeq_calls(model);
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

        return 0;
    };


    // run the simulation
    try {
        osmv::fd_simulation(model, std::move(initial_state), std::move(config));
        shared.wall_end = clock::now().time_since_epoch();
        shared.status = Sim_status::Completed;
    } catch (Stopped_exception const&) {
        shared.wall_end = clock::now().time_since_epoch();
        shared.status = Sim_status::Cancelled;
    } catch (...) {
        shared.wall_end = clock::now().time_since_epoch();
        shared.status = Sim_status::Error;
        throw;
    }

    return 0;
}

// a forward dynamic simulation that is running on a background thread
struct Background_fd_simulation final {
    using clock = std::chrono::steady_clock;

    static constexpr double sim_start_time = 0.0;
    double _sim_final_time = -1337.0;

    Threadsafe_fdsim_state shared;
    shims::jthread worker;
    int states_popped = 0;

    Background_fd_simulation(
            OpenSim::Model const& model,
            SimTK::State const& initial_state,
            double _final_time) :

        _sim_final_time{_final_time},
        shared{osmv::State{initial_state}}
    {
        osmv::Model copy{model};
        osmv::finalize_properties_from_state(copy, initial_state);
        osmv::init_system(copy);

        worker = shims::jthread{
            run_fd_simulation,
            std::move(copy),
            osmv::State{initial_state},
            _sim_final_time,
            std::ref(shared),
        };
    }

    bool try_pop_latest_state(osmv::State& dest) {
        return shared.state.try_apply_b([&](osmv::State& latest) {
            std::swap(dest, latest);
            ++states_popped;
        });
    }

    bool request_stop() noexcept {
        return worker.request_stop();
    }

    bool is_running() const noexcept {
        return shared.status == Sim_status::Running;
    }

    clock::duration wall_duration() const noexcept {
        clock::duration endpoint =
            is_running() ? clock::now().time_since_epoch() : shared.wall_end.load();

        return endpoint - shared.wall_start.load();
    }

    double sim_current_time() const noexcept {
        return shared.sim_cur_time;
    }

    double sim_final_time() const noexcept {
        return _sim_final_time;
    }

    char const* status_description() const noexcept {
        return str(shared.status);
    }

    int num_prescribeq_calls() const noexcept {
        return shared.num_pq_calls;
    }

    double avg_ui_overhead() const noexcept {
        return shared.ui_overhead_acc / shared.ui_overhead_n;
    }

    int num_states_popped() const noexcept {
        return states_popped;
    }
};
