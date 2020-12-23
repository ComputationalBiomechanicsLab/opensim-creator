#pragma once

#include "opensim_wrapper.hpp"

#include <memory>
#include <atomic>
#include <thread>


// shim: std::stop_token (C++20)
class Stop_token final {
    std::shared_ptr<std::atomic<bool>> shared_state;
public:
    Stop_token(std::shared_ptr<std::atomic<bool>> st)
        : shared_state{std::move(st)} {
    }
    Stop_token(Stop_token const&) = delete;
    Stop_token(Stop_token&& tmp) :
        shared_state{tmp.shared_state}  {
    }
    Stop_token& operator=(Stop_token const&) = delete;
    Stop_token& operator=(Stop_token&&) = delete;
    ~Stop_token() noexcept = default;

    bool stop_requested() const noexcept {
        return *shared_state;
    }
};

// shim: std::stop_source (C++20)
class Stop_source final {
    std::shared_ptr<std::atomic<bool>> shared_state;
public:
    Stop_source() :
        shared_state{new std::atomic<bool>{false}} {
    }
    Stop_source(Stop_source const&) = delete;
    Stop_source(Stop_source&& tmp) :
        shared_state{std::move(tmp.shared_state)} {
    }
    Stop_source& operator=(Stop_source const&) = delete;
    Stop_source& operator=(Stop_source&& tmp) {
        shared_state = std::move(tmp.shared_state);
        return *this;
    }
    ~Stop_source() = default;

    bool request_stop() noexcept {
        // as-per the C++20 spec, but always true for this impl.
        bool has_stop_state = shared_state != nullptr;
        bool already_stopped = shared_state->exchange(true);

        return has_stop_state and (not already_stopped);
    }

    Stop_token get_token() const noexcept {
        return Stop_token{shared_state};
    }
};

// sim: std::jthread (C++20)
class Jthread final {
    Stop_source s;
    std::thread t;
public:
    // Creates new thread object which does not represent a thread
    Jthread() :
        s{},
        t{} {
    }

    // Creates new thread object and associates it with a thread of execution.
    // The new thread of execution immediately starts executing
    template<class Function, class... Args>
    Jthread(Function&& f, Args&&... args) :
        s{},
        t{f, s.get_token(), std::forward<Args>(args)...} {
    }

    // threads are non-copyable
    Jthread(Jthread const&) = delete;
    Jthread& operator=(Jthread const&) = delete;

    // threads are moveable: the moved-from value is a non-joinable thread that
    // does not represent a thread
    Jthread(Jthread&& tmp) = default;
    Jthread& operator=(Jthread&&) = default;

    // jthreads (or "joining threads") cancel + join on destruction
    ~Jthread() noexcept {
        if (joinable()) {
            s.request_stop();
            t.join();
        }
    }

    bool joinable() const noexcept {
        return t.joinable();
    }

    bool request_stop() noexcept {
        return s.request_stop();
    }

    void join() {
        return t.join();
    }
};

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

    Passed_parcel<osim::OSMV_State> state;
    std::atomic<double> sim_cur_time = 0.0;
    std::atomic<double> ui_overhead_acc = 0.0;
    std::atomic<clock::duration> wall_start = clock::now().time_since_epoch();
    std::atomic<clock::duration> wall_end = clock::now().time_since_epoch();
    std::atomic<int> num_pq_calls = 0;
    std::atomic<int> ui_overhead_n = 0;
    std::atomic<Sim_status> status = Sim_status::Running;

    Threadsafe_fdsim_state(osim::OSMV_State initial_state) :
        state{std::move(initial_state)} {
    }
};

// runs a forward dynamic simuation, updating `shared` (which is designed to
// be thread-safe) as it runs
int run_fd_simulation(
        Stop_token t,
        osim::OSMV_Model m,
        osim::OSMV_State s,
        double final_time,
        Threadsafe_fdsim_state& shared) {
    class Stopped_exception final {};
    using clock = std::chrono::steady_clock;

    shared.wall_start = clock::now().time_since_epoch();
    shared.status = Sim_status::Running;

    clock::time_point last_report_end = clock::now();

    // what happens during an intermittent update
    auto on_report = [&](osim::Simulation_update_event const& e) {
        if (t.stop_requested()) {
            throw Stopped_exception{};
        }

        clock::time_point report_start = clock::now();

        shared.state.try_apply_a([&e](osim::OSMV_State& s) {
            s = e.state;
        });
        shared.num_pq_calls = e.num_prescribe_q_calls;
        shared.sim_cur_time = e.simulation_time;

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
        osim::fd_simulation(m, s, final_time, std::move(on_report));
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
    Jthread worker;
    int states_popped = 0;

    Background_fd_simulation(
            OpenSim::Model const& model,
            SimTK::State const& initial_state,
            double _final_time) :

        _sim_final_time{_final_time},
        shared{initial_state}
    {
        osim::OSMV_Model copy = osim::copy_model(model);
        osim::finalize_properties_from_state(copy, initial_state);
        osim::init_system(copy);

        worker = Jthread{
            run_fd_simulation,
            std::move(copy),
            osim::OSMV_State{initial_state},
            _sim_final_time,
            std::ref(shared),
        };
    }

    bool try_pop_latest_state(osim::OSMV_State& dest) {
        return shared.state.try_apply_b([&](osim::OSMV_State& latest) {
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
