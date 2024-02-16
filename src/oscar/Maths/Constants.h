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
}
