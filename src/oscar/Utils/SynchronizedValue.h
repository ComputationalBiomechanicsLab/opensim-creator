#pragma once

#include <oscar/Utils/SynchronizedValueGuard.h>

#include <concepts>
#include <mutex>
#include <type_traits>
#include <utility>

namespace osc
{
    // represents a `T` value that can only be accessed via a mutexed guard
    template<
        typename T,
        typename TDefaultGuard = std::lock_guard<std::mutex>
    >
    class SynchronizedValue final {
    public:
        // default-construct T
        SynchronizedValue() = default;

        // in-place constructor for T
        template<typename... Args>
        requires std::constructible_from<T, Args&&...>
        explicit SynchronizedValue(Args&&... args) :
            m_Mutex{},
            m_Value{std::forward<Args>(args)...}
        {}

        SynchronizedValue(const SynchronizedValue& other) :
            m_Mutex{},
            m_Value{*other.lock()}
        {}

        SynchronizedValue(SynchronizedValue&& tmp) noexcept :
            m_Mutex{},
            m_Value{std::move(tmp).value()}
        {}

        SynchronizedValue& operator=(const SynchronizedValue& other)
        {
            if (&other != this) {
                *this->lock() = *other.lock();
            }
            return *this;
        }

        SynchronizedValue& operator=(SynchronizedValue&& tmp) noexcept
        {
            if (&tmp != this) {
                *this->lock() = std::move(tmp).value();
            }
            return *this;
        }

        T value() &&
        {
            const auto guard = std::lock_guard{m_Mutex};
            return std::move(m_Value);
        }

        template<typename TGuard = TDefaultGuard>
        SynchronizedValueGuard<T, TGuard> lock()
        {
            return SynchronizedValueGuard<T, TGuard>{m_Mutex, m_Value};
        }

        template<typename TGuard = TDefaultGuard>
        SynchronizedValueGuard<const T, TGuard> lock() const
        {
            return SynchronizedValueGuard<const T, TGuard>{m_Mutex, m_Value};
        }

        template<
            typename U,
            typename Getter,
            typename TGuard = TDefaultGuard
        >
        SynchronizedValueGuard<const U, TGuard> lockChild(Getter f) const
            requires std::is_same_v<decltype(f(std::declval<T>())), const U&>
        {
            return SynchronizedValueGuard<const U, TGuard>{m_Mutex, f(m_Value)};
        }

    private:
        mutable std::mutex m_Mutex;
        T m_Value{};
    };
}
