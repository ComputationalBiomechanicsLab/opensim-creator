#pragma once

#include <oscar/Utils/Concepts.hpp>
#include <oscar/Utils/SynchronizedValueGuard.hpp>

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
        explicit SynchronizedValue(Args&&... args)
            requires ConstructibleFrom<T, Args&&...> :

            m_Mutex{},
            m_Value{std::forward<Args>(args)...}
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

        T value() &&
        {
            auto const guard = std::lock_guard{m_Mutex};
            return std::move(m_Value);
        }

        template<typename TGuard = TDefaultGuard>
        SynchronizedValueGuard<T, TGuard> lock()
        {
            return SynchronizedValueGuard<T, TGuard>{m_Mutex, m_Value};
        }

        template<typename TGuard = TDefaultGuard>
        SynchronizedValueGuard<T const, TGuard> lock() const
        {
            return SynchronizedValueGuard<T const, TGuard>{m_Mutex, m_Value};
        }

        template<
            typename U,
            typename Getter,
            typename TGuard = TDefaultGuard
        >
        SynchronizedValueGuard<U const, TGuard> lockChild(Getter f) const
            requires std::is_same_v<decltype(f(std::declval<T>())), U const&>
        {
            return SynchronizedValueGuard<U const, TGuard>{m_Mutex, f(m_Value)};
        }

    private:
        mutable std::mutex m_Mutex;
        T m_Value{};
    };
}
