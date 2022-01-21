#pragma once

#include <mutex>
#include <utility>

namespace osc {
    // a mutex guard over a reference to `T`
    template<typename T, typename TGuard = std::lock_guard<std::mutex>>
    class Mutex_guard final {
    public:
        explicit Mutex_guard(std::mutex& mutex, T& _ref) :
            m_Guard{mutex},
            m_Ref{_ref}
        {
        }

        T& operator*() noexcept { return m_Ref; }
        T const& operator*() const noexcept { return m_Ref; }
        T* operator->() noexcept { return &m_Ref; }
        T const* operator->() const noexcept { return &m_Ref; }
        TGuard& raw_guard() noexcept { return m_Guard; }

    private:
        TGuard m_Guard;
        T& m_Ref;
    };

    // represents a `T` value that can only be accessed via a mutex guard
    template<typename T>
    class Mutex_guarded final {
    public:
        explicit Mutex_guarded(T _value) :
            m_Value{std::move(_value)}
        {
        }

        // in-place constructor for T
        template<typename... Args>
        explicit Mutex_guarded(Args... args) :
            m_Value{std::forward<Args...>(args)...}
        {
        }

        Mutex_guard<T> lock()
        {
            return Mutex_guard<T>{m_Mutex, m_Value};
        }

        Mutex_guard<T, std::unique_lock<std::mutex>> unique_lock()
        {
            return Mutex_guard<T, std::unique_lock<std::mutex>>{m_Mutex, m_Value};
        }

    private:
        std::mutex m_Mutex;
        T m_Value;
    };
}
