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

        // in-place constructor for `T`
        template<typename... Args>
        requires std::constructible_from<T, Args&&...>
        explicit SynchronizedValue(Args&&... args) :
            value_mutex_{},
            value_{std::forward<Args>(args)...}
        {}

        SynchronizedValue(const SynchronizedValue& other) :
            value_mutex_{},
            value_{*other.lock()}
        {}

        SynchronizedValue(SynchronizedValue&& tmp) noexcept :
            value_mutex_{},
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

        T value() &&
        {
            const auto guard = std::lock_guard{value_mutex_};
            return std::move(value_);
        }

        template<typename TGuard = TDefaultGuard>
        SynchronizedValueGuard<T, TGuard> lock()
        {
            return SynchronizedValueGuard<T, TGuard>{value_mutex_, value_};
        }

        template<typename TGuard = TDefaultGuard>
        SynchronizedValueGuard<const T, TGuard> lock() const
        {
            return SynchronizedValueGuard<const T, TGuard>{value_mutex_, value_};
        }

        template<
            typename U,
            typename Getter,
            typename TGuard = TDefaultGuard
        >
        SynchronizedValueGuard<const U, TGuard> lock_child(Getter f) const
            requires std::is_same_v<decltype(f(std::declval<T>())), const U&>
        {
            return SynchronizedValueGuard<const U, TGuard>{value_mutex_, f(value_)};
        }

    private:
        mutable std::mutex value_mutex_;
        T value_{};
    };
}
