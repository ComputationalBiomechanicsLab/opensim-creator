#pragma once

#include <oscar/Maths/Angle.h>
#include <oscar/Maths/Constants.h>
#include <oscar/Maths/Functors.h>
#include <oscar/Maths/Mat.h>
#include <oscar/Maths/Vec.h>

#include <cmath>
#include <concepts>
#include <cstddef>
#include <type_traits>

namespace osc
{
    template<std::signed_integral T>
    T abs(T v)
    {
        return std::abs(v);
    }

    template<std::floating_point T>
    T abs(T v)
    {
        return std::fabs(v);
    }

    template<std::floating_point T>
    T floor(T num)
    {
        return std::floor(num);
    }

    template<std::floating_point T>
    T copysign(T mag, T sgn)
    {
        return std::copysign(mag, sgn);
    }

    template<size_t L, typename T>
    Vec<L, T> abs(Vec<L, T> const& v)
        requires std::is_arithmetic_v<T>
    {
        return elementwise_map(v, abs);
    }

    template<std::floating_point T>
    T fmod(T x, T y)
    {
        return std::fmod(x, y);
    }

    template<
        std::floating_point Rep1,
        AngularUnitTraits Units1,
        std::floating_point Rep2,
        AngularUnitTraits Units2
    >
    auto fmod(Angle<Rep1, Units1> x, Angle<Rep2, Units2> y) -> std::common_type_t<decltype(x), decltype(y)>
    {
        using CA = std::common_type_t<decltype(x), decltype(y)>;
        return CA{fmod(CA{x}.count(), CA{y}.count())};
    }

    // returns the smallest of `a` and `b`
    template<typename GenType>
    constexpr GenType min(GenType a, GenType b)
        requires std::is_arithmetic_v<GenType>
    {
        return (b < a) ? b : a;
    }

    template<
        std::floating_point Rep1,
        AngularUnitTraits Units1,
        std::floating_point Rep2,
        AngularUnitTraits Units2
    >
    constexpr auto min(Angle<Rep1, Units1> x, Angle<Rep2, Units2> y) -> std::common_type_t<decltype(x), decltype(y)>
    {
        using CA = std::common_type_t<decltype(x), decltype(y)>;
        return CA{min(CA{x}.count(), CA{y}.count())};
    }

    // returns a vector containing min(a[dim], b[dim]) for each element
    template<size_t L, typename T>
    constexpr Vec<L, T> elementwise_min(Vec<L, T> const& a, Vec<L, T> const& b)
        requires std::is_arithmetic_v<T>
    {
        return elementwise_map(a, b, min);
    }

    // returns the largest of `a` and `b`
    template<typename GenType>
    constexpr GenType max(GenType a, GenType b)
        requires std::is_arithmetic_v<GenType>
    {
        return (a < b) ? b : a;
    }

    // returns a vector containing max(a[i], b[i]) for each element
    template<size_t L, typename T>
    constexpr Vec<L, T> elementwise_max(Vec<L, T> const& a, Vec<L, T> const& b)
        requires std::is_arithmetic_v<T>
    {
        return elementwise_map(a, b, max);
    }

    // clamps `v` between `low` and `high` (inclusive)
    template<typename GenType>
    constexpr GenType clamp(GenType v, GenType low, GenType high)
        requires std::is_arithmetic_v<GenType>
    {
        return min(max(v, low), high);
    }

    // clamps `v` between `low` and `high` (inclusive, all the same unit type)
    template<std::floating_point Rep, AngularUnitTraits Units>
    constexpr Angle<Rep, Units> clamp(
        Angle<Rep, Units> const& v,
        Angle<Rep, Units> const& min,
        Angle<Rep, Units> const& max)
    {
        return Angle<Rep, Units>{clamp(v.count(), min.count(), max.count())};
    }

    // clamps `v` between `low` and `high`
    //
    // `low` and `high` are converted to the units of `v` before clamping
    template<
        std::floating_point Rep,
        AngularUnitTraits Units,
        std::convertible_to<Angle<Rep, Units>> AngleMin,
        std::convertible_to<Angle<Rep, Units>> AngleMax
    >
    constexpr Angle<Rep, Units> clamp(
        Angle<Rep, Units> const& v,
        AngleMin const& min,
        AngleMax const& max)
    {
        return clamp(v, Angle<Rep, Units>{min}, Angle<Rep, Units>{max});
    }

    // clamps each element in `x` between the corresponding elements in `minVal` and `maxVal`
    template<size_t L, typename T>
    constexpr Vec<L, T> clamp(Vec<L, T> const& v, Vec<L, T> const& low, Vec<L, T> const& high)
        requires std::is_arithmetic_v<T>
    {
        return elementwise_min(elementwise_max(v, low), high);
    }

    // clamps each element in `x` between `minVal` and `maxVal`
    template<size_t L, typename T>
    constexpr Vec<L, T> clamp(Vec<L, T> const& v, T const& low, T const& high)
        requires std::is_arithmetic_v<T>
    {
        return clamp(v, Vec<L, T>{low}, Vec<L, T>{high});
    }

    // clamps `v` between 0.0 and 1.0
    template<std::floating_point GenType>
    constexpr GenType saturate(GenType v)
    {
        return clamp(v, static_cast<GenType>(0), static_cast<GenType>(1));
    }

    // clamps each element in `x` between 0.0 and 1.0
    template<size_t L, std::floating_point T>
    constexpr Vec<L, T> saturate(Vec<L, T> const& x)
    {
        return clamp(x, static_cast<T>(0), static_cast<T>(1));
    }

    // linearly interpolates between `x` and `y` with factor `a`
    template<typename GenType, typename UInterpolant>
    constexpr GenType mix(GenType const& x, GenType const& y, UInterpolant const& a)
        requires std::is_arithmetic_v<GenType> && std::is_floating_point_v<UInterpolant>
    {
        return static_cast<GenType>(static_cast<UInterpolant>(x) * (static_cast<UInterpolant>(1) - a) + static_cast<UInterpolant>(y) * a);
    }

    // linearly interpolates between each element in `x` and `y` with factor `a`
    template<size_t L, typename T, typename UInterpolant>
    constexpr Vec<L, T> mix(Vec<L, T> const& x, Vec<L, T> const& y, UInterpolant const& a)
        requires std::is_arithmetic_v<T> && std::is_floating_point_v<UInterpolant>
    {
        return Vec<L, T>(Vec<L, UInterpolant>(x) * (static_cast<UInterpolant>(1) - a) + Vec<L, UInterpolant>(y) * a);
    }

    template<size_t L, typename T>
    constexpr Vec<L, bool> elementwise_less(Vec<L, T> const& x, Vec<L, T> const& y)
    {
        Vec<L, bool> Result(true);
        for (size_t i = 0; i < L; ++i) {
            Result[i] = x[i] < y[i];
        }
        return Result;
    }

    template<size_t L, typename T>
    constexpr Vec<L, bool> elementwise_less(Vec<L, T> const& x, T const& v)
    {
        Vec<L, bool> Result(true);
        for (size_t i = 0; i < L; ++i) {
            Result[i] = x[i] < v;
        }
        return Result;
    }

    template<size_t L, typename T>
    constexpr Vec<L, bool> elementwise_less_equal(Vec<L, T> const& x, Vec<L, T> const& y)
    {
        Vec<L, bool> rv(true);
        for (size_t i = 0; i < L; ++i) {
            rv[i] = x[i] <= y[i];
        }
        return rv;
    }

    template<std::floating_point T>
    bool equal_within_absdiff(T x, T y, T epsilon)
    {
        return abs(x - y) < epsilon;
    }

    template<size_t L, std::floating_point T>
    Vec<L, bool> equal_within_absdiff(Vec<L, T> const& x, Vec<L, T> const& y, Vec<L, T> const& eps)
    {
        return elementwise_less(abs(x - y), eps);
    }

    template<size_t L, std::floating_point T>
    Vec<L, bool> equal_within_absdiff(Vec<L, T> const& x, Vec<L, T> const& y, T const& eps)
    {
        return elementwise_less(abs(x - y), eps);
    }

    template<std::floating_point T>
    bool equal_within_epsilon(T x, T y)
    {
        return equal_within_absdiff(x, y, epsilon<T>);
    }

    template<size_t L, std::floating_point T>
    Vec<L, bool> equal_within_epsilon(Vec<L, T> const& x, Vec<L, T> const& y)
    {
        return equal_within_absdiff(x, y, Vec<L, T>{epsilon<T>});
    }

    template<std::floating_point T>
    bool equal_within_scaled_epsilon(T x, T y)
    {
        // why:
        //
        //     http://realtimecollisiondetection.net/blog/?p=89
        //     https://stackoverflow.com/questions/17333/what-is-the-most-effective-way-for-float-and-double-comparison
        //
        // machine epsilon is only relevant for numbers < 1.0, so the epsilon
        // value must be scaled up to the magnitude of the operands if you need
        // a more-correct equality comparison

        T const scaledEpsilon = max(max(1.0, x), y) * epsilon<T>;
        return abs(x - y) < scaledEpsilon;
    }

    template<std::floating_point T>
    bool equal_within_reldiff(T x, T y, T releps)
    {
        // inspired from:
        //
        // - https://stackoverflow.com/questions/17333/what-is-the-most-effective-way-for-float-and-double-comparison
        //
        // but, specifically, you should read the section `Epsilon comparisons` here:
        //
        // - https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/

        return abs(x - y) <= releps * max(abs(x), abs(y));
    }

    template<size_t L, std::floating_point T>
    Vec<L, bool> equal_within_reldiff(Vec<L, T> const& x, Vec<L, T> const& y, T releps)
    {
        return elementwise_less_equal(abs(x - y), releps * elementwise_max(abs(x), abs(y)));
    }

    template<std::floating_point T>
    T log(T num)
    {
        return std::log(num);
    }

    template<std::floating_point T>
    T pow(T base, T exp)
    {
        return std::pow(base, exp);
    }
}
