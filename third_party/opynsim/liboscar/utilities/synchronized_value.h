#pragma once

#include <liboscar/utilities/synchronized_value_guard.h>

#include <concepts>
#include <mutex>
#include <type_traits>
#include <utility>

namespace osc
{
    // Represents a value, plus an associated mutex, where the value can only be accessed via the mutex.
    template<
        typename T,
        typename Mutex = std::mutex,
        typename MutexGuard = std::lock_guard<Mutex>
    >
    class SynchronizedValue final {
    public:
        using value_type = T;
        using mutex_type = Mutex;
        using mutex_guard_type = MutexGuard;

        // Value-constructs an instance of `value_type` with an associated instance of `mutex_type`.
        SynchronizedValue() = default;

        // In-place constructs an instance of `value_type` from `Args`, along with an associated instance of `mutex_type`.
        template<typename... Args>
        requires std::constructible_from<value_type, Args&&...>
        explicit SynchronizedValue(Args&&... args) :
            value_{std::forward<Args>(args)...}
        {}

        SynchronizedValue(const SynchronizedValue& other) :
            value_{*other.lock()}
        {}

        SynchronizedValue(SynchronizedValue&& tmp) noexcept :
            value_{std::move(tmp).value()}
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

        ~SynchronizedValue() noexcept = default;

        value_type value() &&
        {
            const auto guard = mutex_guard_type{mutex_};
            return std::move(value_);
        }

        template<typename TGuard = MutexGuard>
        SynchronizedValueGuard<value_type, mutex_type, TGuard> lock()
        {
            return SynchronizedValueGuard<value_type, mutex_type, TGuard>{mutex_, value_};
        }

        template<typename TGuard = MutexGuard>
        SynchronizedValueGuard<const value_type, mutex_type, TGuard> lock() const
        {
            return SynchronizedValueGuard<const value_type, mutex_type, TGuard>{mutex_, value_};
        }

        template<
            typename U,
            typename Getter,
            typename TGuard = MutexGuard
        >
        SynchronizedValueGuard<const U, mutex_type, TGuard> lock_child(Getter f) const
            requires std::is_same_v<decltype(f(std::declval<value_type>())), const U&>
        {
            return SynchronizedValueGuard<const U, mutex_type, TGuard>{mutex_, f(value_)};
        }

    private:
        mutable mutex_type mutex_;
        value_type value_{};
    };
}
