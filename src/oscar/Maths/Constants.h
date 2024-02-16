#pragma once

#include <concepts>
#include <limits>
#include <numbers>

namespace osc
{
    template<std::floating_point T>
    static inline constexpr T pi = std::numbers::pi_v<T>;
    static inline constexpr float pi_f = pi<float>;
    static inline constexpr double pi_d = pi<double>;

    template<std::floating_point T>
    static inline constexpr T epsilon = std::numeric_limits<T>::epsilon();
    static inline constexpr float epsilon_f = epsilon<float>;
    static inline constexpr double epsilon_d = epsilon<double>;

    template<std::floating_point T>
    static inline constexpr T quiet_nan = std::numeric_limits<T>::quiet_NaN();
    static inline constexpr float quiet_nan_f = quiet_nan<float>;
    static inline constexpr double quiet_nan_d = quiet_nan<double>;
}
