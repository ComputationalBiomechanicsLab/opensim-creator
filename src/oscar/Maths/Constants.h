#pragma once

#include <concepts>
#include <limits>
#include <numbers>

namespace osc
{
    template<std::floating_point T>
    static inline constexpr T pi = std::numbers::pi_v<T>;

    template<std::floating_point T>
    static inline constexpr T epsilon = std::numeric_limits<T>::epsilon();

    template<std::floating_point T>
    static inline constexpr T quiet_nan = std::numeric_limits<T>::quiet_NaN();
}
