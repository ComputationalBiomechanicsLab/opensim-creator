#pragma once

#include <mutex>
#include <utility>

namespace osc
{
    // accessor to a reference to the guarded value
    template<typename T, typename TGuard = std::lock_guard<std::mutex>>
    class SynchronizedValueGuard final {
    public:
        SynchronizedValueGuard(std::mutex& mutex, T& _ref) :
            m_Guard{mutex},
            m_Ref{_ref}
        {
        }
        SynchronizedValueGuard(SynchronizedValueGuard const&) = delete;
        SynchronizedValueGuard(SynchronizedValueGuard&&) noexcept = delete;
        SynchronizedValueGuard& operator=(SynchronizedValueGuard const&) = delete;
        SynchronizedValueGuard& operator=(SynchronizedValueGuard&&) noexcept = delete;
        ~SynchronizedValueGuard() noexcept = default;

        T& operator*() & noexcept { return m_Ref; }
        T const& operator*() const & noexcept { return m_Ref; }
        T* operator->() noexcept { return &m_Ref; }
        T const* operator->() const noexcept { return &m_Ref; }

    private:
        TGuard m_Guard;
        T& m_Ref;
    };

    // represents a `T` value that can only be accessed via a mutexed guard
    template<typename T>
    class SynchronizedValue final {
    public:
        // default-construct T
        SynchronizedValue() = default;

        // in-place constructor for T
        template<typename... Args>
        explicit SynchronizedValue(Args... args) :
            m_Value{std::forward<Args...>(args)...}
        {
        }

        SynchronizedValue(SynchronizedValue const& other) :
            m_Mutex{},
            m_Value{*other.lock()}
        {
        }

        SynchronizedValue(SynchronizedValue&& tmp) :
            m_Mutex{},
            m_Value{std::move(tmp).value()}
        {
        }

        SynchronizedValue& operator=(SynchronizedValue const& other)
        {
            if (&other != this)
            {
                *this->lock() = *other.lock();
            }
            return *this;
        }

        SynchronizedValue& operator=(SynchronizedValue&& tmp) noexcept
        {
            if (&tmp != this)
            {
                *this->lock() = std::move(tmp).value();
            }
            return *this;
        }

        T&& value() &&
        {
            auto guard = std::lock_guard{m_Mutex};
            return std::move(m_Value);
        }

        std::mutex& mutex() const
        {
            return m_Mutex;
        }

        SynchronizedValueGuard<T> lock()
        {
            return SynchronizedValueGuard<T>{m_Mutex, m_Value};
        }

        SynchronizedValueGuard<T const> lock() const
        {
            return SynchronizedValueGuard<T const>{m_Mutex, m_Value};
        }

        SynchronizedValueGuard<T, std::unique_lock<std::mutex>> unique_lock()
        {
            return SynchronizedValueGuard<T, std::unique_lock<std::mutex>>{m_Mutex, m_Value};
        }

    private:
        mutable std::mutex m_Mutex;
        T m_Value;
    };
}
