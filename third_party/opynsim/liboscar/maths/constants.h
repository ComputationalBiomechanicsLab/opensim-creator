#pragma once

#include <concepts>
#include <limits>
#include <numbers>

namespace osc
{
    // the mathematical constant, pi
    template<std::floating_point T>
    static inline constexpr T pi_v = std::numbers::pi_v<T>;

    // the difference between `T{1}` and the next representable value of the given floating-point type
    template<std::floating_point T>
    static inline constexpr T epsilon_v = std::numeric_limits<T>::epsilon();

    // a quiet Not-a-Number (NaN) value of the given floating-point type
    template<std::floating_point T>
    static inline constexpr T quiet_nan_v = std::numeric_limits<T>::quiet_NaN();

    // A multiplier that converts a degree value of type `T` to radians.
    template<std::floating_point T>
    static inline constexpr T deg_to_rad_v = pi_v<T> / T{180};

    // A multiplier that converts a radians value of type `T` to degrees.
    template<std::floating_point T>
    static inline constexpr T rad_to_deg_v = T{180} / pi_v<T>;
}
