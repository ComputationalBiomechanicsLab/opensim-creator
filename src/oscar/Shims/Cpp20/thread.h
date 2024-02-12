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
        // Creates new thread object which does not represent a thread
        jthread() = default;

        // Creates new thread object and associates it with a thread of execution.
        // The new thread of execution immediately starts executing
        template<typename Function, typename... Args>
        jthread(Function&& f, Args&&... args)
            requires std::invocable<Function, stop_token, Args&&...> :

            m_StopSource{},
            m_Thread{std::forward<Function>(f), m_StopSource.get_token(), std::forward<Args>(args)...}
        {
        }

        // threads are non-copyable
        jthread(jthread const&) = delete;
        jthread& operator=(jthread const&) = delete;

        // threads are moveable: the moved-from value is a non-joinable thread that
        // does not represent a thread
        jthread(jthread&&) noexcept = default;

        jthread& operator=(jthread&& other) noexcept
        {
            if (joinable())
            {
                m_StopSource.request_stop();
                m_Thread.join();
            }
            std::swap(m_StopSource, other.m_StopSource);
            std::swap(m_Thread, other.m_Thread);
            return *this;
        }

        // jthreads (or "joining threads") cancel + join on destruction
        ~jthread() noexcept
        {
            if (joinable())
            {
                m_StopSource.request_stop();
                m_Thread.join();
            }
        }

        bool joinable() const noexcept
        {
            return m_Thread.joinable();
        }

        bool request_stop() noexcept
        {
            return m_StopSource.request_stop();
        }

        void join()
        {
            return m_Thread.join();
        }

    private:
        stop_source m_StopSource;
        std::thread m_Thread;
    };
}
