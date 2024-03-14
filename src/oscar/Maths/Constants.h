#pragma once

#include <concepts>
#include <limits>
#include <numbers>

namespace osc
{
    // the mathematical constant, pi
    template<std::floating_point T>
    static inline constexpr T pi_v = std::numbers::pi_v<T>;

    // the difference between T{1} and the next representable value of the given floating-point type
    template<std::floating_point T>
    static inline constexpr T epsilon_v = std::numeric_limits<T>::epsilon();

    // a quiet Not-a-Number (NaN) value of the given floating-point type
    template<std::floating_point T>
    static inline constexpr T quiet_nan_v = std::numeric_limits<T>::quiet_NaN();
}
