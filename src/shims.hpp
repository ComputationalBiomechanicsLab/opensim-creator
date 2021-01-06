#pragma once

#include <memory>
#include <atomic>
#include <thread>

// shims: *roughly* compatible shims to features available in newer C++es
namespace shims {

    // C++20: std::stop_token
    class stop_token final {
        std::shared_ptr<std::atomic<bool>> shared_state;
    public:
        stop_token(std::shared_ptr<std::atomic<bool>> st)
            : shared_state{ std::move(st) } {
        }
        stop_token(stop_token const&) = delete;
        stop_token(stop_token&& tmp) :
            shared_state{ tmp.shared_state } {
        }
        stop_token& operator=(stop_token const&) = delete;
        stop_token& operator=(stop_token&&) = delete;
        ~stop_token() noexcept = default;

        bool stop_requested() const noexcept {
            return *shared_state;
        }
    };

    // C++20: std::stop_source
    class stop_source final {
        std::shared_ptr<std::atomic<bool>> shared_state;
    public:
        stop_source() :
            shared_state{ new std::atomic<bool>{false} } {
        }
        stop_source(stop_source const&) = delete;
        stop_source(stop_source&& tmp) :
            shared_state{ std::move(tmp.shared_state) } {
        }
        stop_source& operator=(stop_source const&) = delete;
        stop_source& operator=(stop_source&& tmp) {
            shared_state = std::move(tmp.shared_state);
            return *this;
        }
        ~stop_source() = default;

        bool request_stop() noexcept {
            // as-per the C++20 spec, but always true for this impl.
            bool has_stop_state = shared_state != nullptr;
            bool already_stopped = shared_state->exchange(true);

            return has_stop_state and (not already_stopped);
        }

        stop_token get_token() const noexcept {
            return stop_token{ shared_state };
        }
    };

    // C++20: std::jthread
    class jthread final {
        stop_source s;
        std::thread t;
    public:
        // Creates new thread object which does not represent a thread
        jthread() :
            s{},
            t{} {
        }

        // Creates new thread object and associates it with a thread of execution.
        // The new thread of execution immediately starts executing
        template<class Function, class... Args>
        jthread(Function&& f, Args&&... args) :
            s{},
            t{ f, s.get_token(), std::forward<Args>(args)... } {
        }

        // threads are non-copyable
        jthread(jthread const&) = delete;
        jthread& operator=(jthread const&) = delete;

        // threads are moveable: the moved-from value is a non-joinable thread that
        // does not represent a thread
        jthread(jthread&& tmp) = default;
        jthread& operator=(jthread&&) = default;

        // jthreads (or "joining threads") cancel + join on destruction
        ~jthread() noexcept {
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
}
