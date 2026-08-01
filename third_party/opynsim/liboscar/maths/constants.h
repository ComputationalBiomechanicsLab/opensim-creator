#pragma once

#include <concepts>
#include <limits>
#include <numbers>

namespace osc
{
    namespace detail
    {
        // This stuff is necessary because `std::sqrt` isn't `constexpr` yet.

        template<typename T>
        struct sqrt_epsilon_impl;

        template<>
        struct sqrt_epsilon_impl<float> { static constexpr float value = 0.00034526698f; };

        template<>
        struct sqrt_epsilon_impl<double> { static constexpr double value = 0.000000014901161193847656; };
    }

    // the mathematical constant, pi
    template<std::floating_point T>
    inline constexpr T pi_v = std::numbers::pi_v<T>;

    // the difference between `T{1}` and the next representable value of the given floating-point type
    template<std::floating_point T>
    inline constexpr T epsilon_v = std::numeric_limits<T>::epsilon();

    // The square root of `epsilon_v` for `T`
    template<std::floating_point T>
    inline constexpr T sqrt_epsilon_v = detail::sqrt_epsilon_impl<T>::value;

    // a quiet Not-a-Number (NaN) value of the given floating-point type
    template<std::floating_point T>
    inline constexpr T quiet_nan_v = std::numeric_limits<T>::quiet_NaN();

    // A multiplier that converts a degree value of type `T` to radians.
    template<std::floating_point T>
    inline constexpr T deg_to_rad_v = pi_v<T> / T{180};

    // A multiplier that converts a radians value of type `T` to degrees.
    template<std::floating_point T>
    inline constexpr T rad_to_deg_v = T{180} / pi_v<T>;

    // The maximum absolute dot product between two unit vectors such that
    // normalizing their cross-product remains numerically stable.
    template<std::floating_point T>
    inline constexpr T max_abs_dot_for_cross_v = T(1) - sqrt_epsilon_v<T>;
}
