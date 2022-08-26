#pragma once

#include <mutex>
#include <type_traits>
#include <utility>

namespace osc
{
    // accessor to a reference to the guarded value
    template<typename T, typename TGuard = std::lock_guard<std::mutex>>
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

        SynchronizedValue(SynchronizedValue&& tmp) noexcept :
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

        template<typename TGuard = std::lock_guard<std::mutex>>
        SynchronizedValueGuard<T, TGuard> lock()
        {
            return SynchronizedValueGuard<T, TGuard>{m_Mutex, m_Value};
        }

        template<typename TGuard = std::lock_guard<std::mutex>>
        SynchronizedValueGuard<T const, TGuard> lock() const
        {
            return SynchronizedValueGuard<T const, TGuard>{m_Mutex, m_Value};
        }

        template<typename U, typename Getter, typename TGuard = std::lock_guard<std::mutex>>
        SynchronizedValueGuard<U const, TGuard> lockChild(Getter f) const
        {
            static_assert(std::is_same_v<decltype(f(std::declval<T>())), U const&>, "getter function should return reference to inner child");
            return SynchronizedValueGuard<U const, TGuard>{m_Mutex, f(m_Value)};
        }

    private:
        mutable std::mutex m_Mutex;
        T m_Value;
    };
}
