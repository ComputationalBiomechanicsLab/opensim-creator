#pragma once

#include <oscar/Maths/CommonFunctions.h>

#include <cmath>
#include <concepts>
#include <cstddef>
#include <optional>
#include <utility>

namespace osc
{
    // vocabulary type to describe "two fixed endpoints with no 'gaps', including the endpoints themselves"
    template<typename T>
    requires std::equality_comparable<T> and std::totally_ordered<T>
    struct ClosedInterval final {

        constexpr ClosedInterval() = default;

        constexpr ClosedInterval(T lower_, T upper_) :
            lower{std::move(lower_)},
            upper{std::move(upper_)}
        {}

        friend bool operator==(const ClosedInterval&, const ClosedInterval&) = default;

        // returns the diameter of a discrete step that satisfies the following equation:
        //
        //     lower + nsteps * step_size(nsteps) == upper
        //
        // such that it's compatible with 0-indexed discretization, i.e.:
        //
        //     for (int step = 0; step < nsteps; ++step) {
        //         // first iteration:                  `value == lower`
        //         // last iteration (if nsteps > 1):   `value == upper`
        //         T value = lower + step * step_size(nsteps);
        //     }
        //
        // i.e. it describes how the `ClosedInterval` should be discretized while
        //      including the endpoints
        template<std::integral U>
        constexpr T step_size(U nsteps) const
        {
            if (nsteps <= 1) {
                return upper - lower;  // edge-case
            }

            return (upper - lower) / static_cast<T>((static_cast<std::make_unsigned_t<U>>(nsteps) - 1));
        }

        // returns the equivalent normalized interpolant that could be used as an argument
        // to `lerp` between the interval's endpoints. E.g.:
        //
        // - normalized_interpolant_at(lower) == 0.0f
        // - normalized_interpolant_at(upper) == 1.0f
        //
        // an out-of-bounds argument behaves as-if `lerp`ing along the line created between
        // `lower` and `upper`
        constexpr T normalized_interpolant_at(T v) const
        {
            if (lower == upper) {
                return T{0};  // the inverse of `std::lerp`'s behavior (ignoring `std::isfinite`)
            }
            else {
                return (v - lower) / (upper - lower);
            }
        }

        // returns the absolute difference between the endpoints
        constexpr T length() const
        {
            return abs(upper - lower);
        }

        // returns `length() / 2`
        constexpr T half_length() const
        {
            return length() / T{2};
        }

        constexpr bool contains(T v) const
        {
            return lower <= v and v <= upper;
        }

        T lower{};
        T upper{};
    };

    // returns the unit interval for the given floating-point `T`
    template<std::floating_point T>
    constexpr ClosedInterval<T> unit_interval()
    {
        return ClosedInterval<T>(T{0}, T{1});
    }

    // returns a `ClosedInterval<T>` with `lower == interval.lower - abs_amount` and
    // `upper == interval.upper + abs_amount`
    template<typename T>
    constexpr ClosedInterval<T> expand_by_absolute_amount(const ClosedInterval<T>& interval, T abs_amount)
    {
        return {interval.lower - abs_amount, interval.upper + abs_amount};
    }

    // returns a `ClosedInterval` that tightly bounds `x`
    template<typename T>
    constexpr ClosedInterval<T> bounding_interval_of(const T& x)
    {
        return ClosedInterval<T>{x, x};
    }

    // returns a `ClosedInterval` that tightly bounds both `x` and `y`
    template<typename T>
    constexpr ClosedInterval<T> bounding_interval_of(const ClosedInterval<T>& x, const T& y)
    {
        return ClosedInterval<T>{min(x.lower, y), max(x.upper, y)};
    }

    // returns a `ClosedInterval` that tightly bounds both `x` (if present) and `y`
    template<typename T>
    constexpr ClosedInterval<T> bounding_interval_of(const std::optional<ClosedInterval<T>>& x, const T& y)
    {
        return x ? bounding_interval_of(*x, y) : bounding_interval_of(y);
    }
}
