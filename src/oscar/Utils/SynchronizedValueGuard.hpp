#pragma once

#include <mutex>

namespace osc
{
    // accessor to a reference to the guarded value
    template<
        typename T,
        typename TGuard = std::lock_guard<std::mutex>
    >
    class SynchronizedValueGuard final {
    public:
        SynchronizedValueGuard(std::mutex& mutex, T& _ref) :
            m_Guard{mutex},
            m_Ptr{&_ref}
        {
        }

        T& operator*() & noexcept { return *m_Ptr; }
        T const& operator*() const & noexcept { return *m_Ptr; }
        T* operator->() noexcept { return m_Ptr; }
        T const* operator->() const noexcept { return m_Ptr; }

    private:
        TGuard m_Guard;
        T* m_Ptr;
    };
}
