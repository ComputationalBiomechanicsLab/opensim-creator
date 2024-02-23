#pragma once

#include <concepts>
#include <limits>
#include <numbers>

namespace osc
{
    template<std::floating_point T>
    static inline constexpr T pi_v = std::numbers::pi_v<T>;

    template<std::floating_point T>
    static inline constexpr T epsilon_v = std::numeric_limits<T>::epsilon();

    template<std::floating_point T>
    static inline constexpr T quiet_nan_v = std::numeric_limits<T>::quiet_NaN();
}
