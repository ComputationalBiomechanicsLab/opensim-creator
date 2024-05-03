#pragma once

#include <oscar/Shims/Cpp20/stop_token.h>

#include <concepts>
#include <thread>
#include <utility>

namespace osc::cpp20
{
    // C++20: std::jthread
    class jthread final {
    public:
        // initializes a new thread object which does not represent a thread
        jthread() = default;

        // initializes a new thread object and associates it with a thread of execution. The
        // new thread of execution immediately starts executing
        template<typename Function, typename... Args>
        requires std::invocable<Function, stop_token, Args&&...>
        jthread(Function&& f, Args&&... args) :
            stop_source_{},
            thread_{std::forward<Function>(f), stop_source_.get_token(), std::forward<Args>(args)...}
        {}

        // threads are non-copyable
        jthread(jthread const&) = delete;
        jthread& operator=(jthread const&) = delete;

        // threads are moveable: the moved-from value is a non-joinable thread that
        // does not represent a thread
        jthread(jthread&&) noexcept = default;

        jthread& operator=(jthread&& other) noexcept
        {
            if (joinable()) {
                stop_source_.request_stop();
                thread_.join();
            }
            std::swap(stop_source_, other.stop_source_);
            std::swap(thread_, other.thread_);
            return *this;
        }

        // jthreads (or "joining threads") cancel + join on destruction
        ~jthread() noexcept
        {
            if (joinable()) {
                stop_source_.request_stop();
                thread_.join();
            }
        }

        bool joinable() const noexcept
        {
            return thread_.joinable();
        }

        bool request_stop() noexcept
        {
            return stop_source_.request_stop();
        }

        void join()
        {
            return thread_.join();
        }

    private:
        stop_source stop_source_;
        std::thread thread_;
    };
}
