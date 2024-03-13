#pragma once

#include <oscar/Maths/Constants.h>
#include <oscar/Maths/Functors.h>
#include <oscar/Maths/Vec.h>
#include <oscar/Utils/Algorithms.h>

#include <algorithm>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <numeric>
#include <ranges>
#include <span>
#include <tuple>
#include <type_traits>

namespace osc
{
    // returns the absolute value of `num`
    template<std::signed_integral T>
    T abs(T num)
    {
        return std::abs(num);
    }

    // returns the absolute value of `num`
    template<std::floating_point T>
    T abs(T num)
    {
        return std::fabs(num);
    }

    // satisfied if `abs(x)` is a valid expression for type `T`
    template<typename T>
    concept HasAbsFunction = requires (T x) {
        { abs(x) } -> std::same_as<T>;
    };

    // returns a vector containing `abs(xv)` for each `xv` in `x`
    template<size_t L, HasAbsFunction T>
    Vec<L, T> abs(Vec<L, T> const& x)
    {
        return map(x, [](T const& xv) { return abs(xv); });
    }

    // returns the largest integer value not greater than `num`
    template<std::floating_point T>
    T floor(T num)
    {
        return std::floor(num);
    }

    // satisfied if `floor(x)` is a valid expression for type `T`
    template<typename T>
    concept HasFloorFunction = requires (T x) {
        { floor(x) } -> std::same_as<T>;
    };

    // returns a vector containing `floor(xv)` for each `xv` in `x`
    template<size_t L, HasFloorFunction T>
    Vec<L, T> floor(Vec<L, T> const& x)
    {
        return map(x, [](T const& xv) { return floor(xv); });
    }

    // returns a floating point value with magnitude `mag` and the sign of `sgn`
    template<std::floating_point T>
    T copysign(T mag, T sgn)
    {
        return std::copysign(mag, sgn);
    }

    // returns the integer remainder of the division operation `x / y`
    template<std::integral T>
    constexpr T mod(T x, T y)
    {
        return x % y;
    }

    // returns the floating-point remainder of the division operation `x / y`
    template<std::floating_point T>
    T mod(T x, T y)
    {
        return std::fmod(x, y);
    }

    // satisfied if `mod(x, y)` is a valid expression for type `T`
    template<typename T>
    concept HasModFunction = requires (T x, T y) {
        { mod(x, y) } -> std::same_as<T>;
    };

    // returns a vector containing `mod(xv, yv)` for each pair `(xv, yv)` in `x` and `y`
    template<size_t L, HasModFunction T>
    constexpr Vec<L, T> mod(Vec<L, T> const& x, Vec<L, T> const& y)
    {
        return map(x, y, [](T const& xv, T const& yv) { return mod(xv, yv); });
    }

    // satisfied if `min(x, y)` is a valid expression for type `T`
    template<typename T>
    concept HasMinFunction = requires(T x, T y) {
        { min(x, y) } -> std::convertible_to<T>;
    };

    // returns a vector containing `min(xv, yv)` for each `(xv, yv)` in `x` and `y`
    template<size_t L, HasMinFunction T>
    constexpr Vec<L, T> elementwise_min(Vec<L, T> const& x, Vec<L, T> const& y)
    {
        return map(x, y, [](T const& xv, T const& yv) { return min(xv, yv); });
    }

    // satisfied if `max(x, y)` is a valid expression for type `T`
    template<typename T>
    concept HasMaxFunction = requires(T x, T y) {
        { max(x, y) } -> std::convertible_to<T>;
    };

    // returns a vector containing `max(xv, yv)` for each `(xv, yv)` in `x` and `y`
    template<size_t L, HasMaxFunction T>
    constexpr Vec<L, T> elementwise_max(Vec<L, T> const& x, Vec<L, T> const& y)
        requires std::is_arithmetic_v<T>
    {
        return map(x, y, [](T const& xv, T const& yv) { return max(xv, yv); });
    }

    // satisfied if `clamp(v, lo, hi)` is a valid expression for type `T`
    template<typename T>
    concept HasClampFunction = requires(T v, T lo, T hi) {
        { clamp(v, lo, hi) } -> std::convertible_to<T>;
    };

    // returns a vector containing `clamp(vv, lowv, hiv)` for each `(vv, lowv, hiv)` in `v`, `low`, and `hi`
    template<size_t L, HasClampFunction T>
    constexpr Vec<L, T> clamp(Vec<L, T> const& v, Vec<L, T> const& lo, Vec<L, T> const& hi)
    {
        return map(v, lo, hi, [](T const& vv, T const& lov, T const& hiv) { return clamp(vv, lov, hiv); });
    }

    // returns a vector containing `clamp(vv, lo, hi)` for each `vv` in `v`
    template<size_t L, HasClampFunction T>
    constexpr Vec<L, T> clamp(Vec<L, T> const& v, T const& lo, T const& hi)
    {
        return map(v, [&lo, &hi](T const& vv) { return clamp(vv, lo, hi); });
    }

    // returns `clamp(num, T{0}, T{1})`
    template<std::floating_point T>
    constexpr T saturate(T num)
    {
        return clamp(num, static_cast<T>(0), static_cast<T>(1));
    }

    // satisfied if `saturate(x)` is a valid expression for type `T`
    template<typename T>
    concept HasSaturateFunction = requires(T x) {
        { saturate(x) } -> std::convertible_to<T>;
    };

    // returns a vector containing `saturate(xv)` for each `xv` in `x`
    template<size_t L, HasSaturateFunction T>
    constexpr Vec<L, T> saturate(Vec<L, T> const& x)
    {
        return map(x, [](T const& xv) { return saturate(xv); });
    }

    // returns the equivalent of `a + t(b - a)` (linear interpolation with extrapolation)
    template<
        typename Arithmetic1,
        typename Arithmetic2,
        typename Arithmetic3
    >
    constexpr auto lerp(Arithmetic1 const& a, Arithmetic2 const& b, Arithmetic3 const& t)
        requires std::is_arithmetic_v<Arithmetic1> && std::is_arithmetic_v<Arithmetic2> && std::is_arithmetic_v<Arithmetic3>
    {
        return std::lerp(a, b, t);
    }

    // satisfied if `lerp(a, b, t)` is a valid expression for type `T` and interpolant type `Arithmetic`
    template<typename T, typename TInterpolant>
    concept HasLerpFunction = requires(T a, T b, TInterpolant t) {
        { lerp(a, b, t) } -> std::convertible_to<T>;
    };

    // returns a vector containing `lerp(xv, yv, t)` for each `(xv, yv)` in `x` and `y`
    template<size_t L, typename T, typename TInterpolant>
    constexpr auto lerp(Vec<L, T> const& x, Vec<L, T> const& y, TInterpolant const& t) -> Vec<L, decltype(lerp(x[0], y[0], t))>
        requires HasLerpFunction<T, TInterpolant>
    {
        return map(x, y, [&t](T const& xv, T const& yv) { return lerp(xv, yv, t); });
    }

    // returns a vector containing `xv == yv` for each `(xv, yv)` in `x` and `y`
    template<size_t L, std::equality_comparable T>
    constexpr Vec<L, bool> elementwise_equal(Vec<L, T> const& x, Vec<L, T> const& y)
    {
        return map(x, y, std::equal_to<T>{});
    }

    // satisfied if `x < y` is a valid expression that yields a `bool`
    template<typename T>
    concept LessThanComparable = requires(T x, T y) {
        { x < y } -> std::convertible_to<bool>;
    };

    // returns a vector containing `xv < yv` for each `(xv, yv)` in `x` and `y`
    template<size_t L, LessThanComparable T>
    constexpr Vec<L, bool> elementwise_less(Vec<L, T> const& x, Vec<L, T> const& y)
    {
        return map(x, y, std::less<T>{});
    }

    // returns a vector containing `xv < v` for each `xv` in `x`
    template<size_t L, LessThanComparable T>
    constexpr Vec<L, bool> elementwise_less(Vec<L, T> const& x, T const& v)
    {
        return map(x, [&v](T const& el) { return std::less<T>{}(el, v); });
    }

    // satisfied if `x <= y` is a valid expression that yields a `bool`
    template<typename T>
    concept LessThanOrEqualToComparable = requires(T x, T y) {
        { x <= y } -> std::convertible_to<bool>;
    };

    // returns a vector containing `xv <= yv` for each `(xv, yv)` in `x` and `y`
    template<size_t L, LessThanOrEqualToComparable T>
    constexpr Vec<L, bool> elementwise_less_equal(Vec<L, T> const& x, Vec<L, T> const& y)
    {
        return map(x, y, std::less_equal<T>{});
    }

    // tests if the absolute difference between `x` and `y` is less than `absdiff`
    template<typename T>
    bool equal_within_absdiff(T x, T y, T absdiff)
        requires HasAbsFunction<T> && LessThanComparable<T>
    {
        return abs(x - y) < absdiff;
    }

    // returns a vector containing `equal_within_absdiff(xv, yv, absdiffv)` for each `(xv, yv, absdiffv)` in `x`, `y`, and `absdiff`
    template<size_t L, typename T>
    Vec<L, bool> equal_within_absdiff(Vec<L, T> const& x, Vec<L, T> const& y, Vec<L, T> const& absdiff)
        requires HasAbsFunction<T> && LessThanComparable<T>
    {
        return map(x, y, absdiff, equal_within_absdiff<T>);
    }

    // returns a vector containing `equal_within_absdiff(xv, yv, absdiff)` for each `(xv, yv)` in `x` and `y`
    template<size_t L, typename T>
    Vec<L, bool> equal_within_absdiff(Vec<L, T> const& x, Vec<L, T> const& y, T const& absdiff)
        requires HasAbsFunction<T> && LessThanComparable<T>
    {
        return elementwise_less(abs(x - y), absdiff);
    }

    // tests if the absolute difference between `x` and `y` is less than epsilon (machine precision)
    template<std::floating_point T>
    bool equal_within_epsilon(T x, T y)
    {
        return equal_within_absdiff(x, y, epsilon_v<T>);
    }

    // returns a vector containing `equal_within_epsilon(xv, yv)` for each `(xv, yv)` in `x` and `y`
    template<size_t L, std::floating_point T>
    Vec<L, bool> equal_within_epsilon(Vec<L, T> const& x, Vec<L, T> const& y)
    {
        return equal_within_absdiff(x, y, epsilon_v<T>);
    }

    // tests if the absolute difference between `x` and `y` is less than epsilon (machine precision) after
    // accounting for scaling epsilon to the the magnitude of `x` and `y`
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

        T const scaledEpsilon = max(static_cast<T>(1.0), x, y) * epsilon_v<T>;
        return abs(x - y) < scaledEpsilon;
    }

    // tests if the relative difference between `x` and `y` is less than `reldiff` (fraction)
    template<std::floating_point T>
    bool equal_within_reldiff(T x, T y, T reldiff)
    {
        // inspired from:
        //
        // - https://stackoverflow.com/questions/17333/what-is-the-most-effective-way-for-float-and-double-comparison
        //
        // but, specifically, you should read the section `Epsilon comparisons` here:
        //
        // - https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/

        return abs(x - y) <= reldiff * max(abs(x), abs(y));
    }

    // returns a vector containing `equal_within_reldiff(xv, yv, reldiff)` for each `(xv, yv)` in `x` and `y`
    template<size_t L, std::floating_point T>
    Vec<L, bool> equal_within_reldiff(Vec<L, T> const& x, Vec<L, T> const& y, T reldiff)
    {
        return map(x, y, Vec<L, T>(reldiff), equal_within_reldiff<T>);
    }

    // tests if `num` is NaN
    template<std::floating_point T>
    bool isnan(T num)
    {
        return std::isnan(num);
    }

    // returns a vector containing `isnan(xv)` for each `xv` in `x`
    template<size_t L, std::floating_point T>
    Vec<L, bool> isnan(Vec<L, T> const& x)
    {
        return map(x, isnan<T>);
    }

    // returns the natural (base e) logarithm of `num`
    template<std::floating_point T>
    T log(T num)
    {
        return std::log(num);
    }

    // returns the value of `base` raised to the power of `exp`
    template<std::floating_point T>
    T pow(T base, T exp)
    {
        return std::pow(base, exp);
    }

    // returns the midpoint between `a` and `b` while accounting for overflow
    template<typename T>
    constexpr T midpoint(T a, T b)
        requires std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_pointer_v<T>
    {
        return std::midpoint(a, b);
    }

    // satisfied if `midpoint(x, y)` is a valid expresssion for type `T`
    template<typename T>
    concept HasMidpointFunction = requires(T x, T y) {
        { midpoint(x, y) } -> std::convertible_to<T>;
    };

    // returns a vector containing `midpoint(xv, yv)` for each `(xv, yv)` in `x` and `y`
    template<size_t L, typename T>
    Vec<L, T> midpoint(Vec<L, T> const& x, Vec<L, T> const& y)
    {
        return map(x, y, [](T const& xv, T const& yv) { return midpoint(xv, yv); });
    }

    // returns the arithmetic mean of the provided vectors, or `Vec<L, T>{}/T{0}` if provided no vectors
    template<
        std::ranges::range R,
        size_t L = std::tuple_size_v<typename std::ranges::range_value_t<R>>,
        typename T = typename std::ranges::range_value_t<R>::value_type
    >
    constexpr Vec<L, T> centroid(R const& r)
        requires std::is_arithmetic_v<T>
    {
        return std::reduce(std::ranges::begin(r), std::ranges::end(r)) / static_cast<T>(std::ranges::size(r));
    }

    // returns the arithmetic mean of the provided vectors, or `Vec<L, T>{}/T{0}` if provided no vectors
    template<size_t L, typename T>
    constexpr Vec<L, T> centroid(std::initializer_list<Vec<L, T>> const& vs)
        requires std::is_arithmetic_v<T>
    {
        return centroid(std::span<Vec<L, T> const>{vs});
    }
}
