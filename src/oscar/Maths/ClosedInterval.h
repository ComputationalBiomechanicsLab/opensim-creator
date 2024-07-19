#pragma once

#include <oscar/Utils/Assertions.h>

#include <concepts>
#include <cstddef>
#include <utility>

namespace osc
{
    // vocabulary type to describe "two fixed endpoints with no 'gaps', including the endpoints themselves"
    template<typename T>
    requires std::equality_comparable<T> and std::totally_ordered<T>
    struct ClosedInterval final {

        constexpr ClosedInterval(T lower_, T upper_) :
            lower{std::move(lower_)},
            upper{std::move(upper_)}
        {}

        friend bool operator==(const ClosedInterval&, const ClosedInterval&) = default;

        template<std::integral U>
        constexpr T step_size(U nsteps) const
        {
            if (nsteps <= 1) {
                return upper - lower;  // edge-case
            }

            return (upper - lower) / static_cast<T>((static_cast<std::make_unsigned_t<U>>(nsteps) - 1));
        }

        constexpr T normalized_interpolant_at(T v) const
        {
            return (v - lower) / (upper - lower);
        }

        T lower;
        T upper;
    };
}
