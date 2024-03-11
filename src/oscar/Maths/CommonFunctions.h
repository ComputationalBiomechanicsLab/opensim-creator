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
    template<std::signed_integral T>
    T abs(T num)
    {
        return std::abs(num);
    }

    template<std::floating_point T>
    T abs(T num)
    {
        return std::fabs(num);
    }

    template<typename T>
    concept HasAbsFunction = requires (T x) {
        { abs(x) } -> std::same_as<T>;
    };

    template<size_t L, HasAbsFunction T>
    Vec<L, T> abs(Vec<L, T> const& x)
    {
        return map(x, [](T const& xv) { return abs(xv); });
    }

    template<std::floating_point T>
    T floor(T num)
    {
        return std::floor(num);
    }

    template<typename T>
    concept HasFloorFunction = requires (T x) {
        { floor(x) } -> std::same_as<T>;
    };

    template<size_t L, HasFloorFunction T>
    Vec<L, T> floor(Vec<L, T> const& x)
    {
        return map(x, [](T const& xv) { return floor(xv); });
    }

    template<std::floating_point T>
    T copysign(T mag, T sgn)
    {
        return std::copysign(mag, sgn);
    }

    template<std::integral T>
    constexpr T mod(T x, T y)
    {
        return x % y;
    }

    template<std::floating_point T>
    T mod(T x, T y)
    {
        return std::fmod(x, y);
    }

    template<typename T>
    concept HasModFunction = requires (T x, T y) {
        { mod(x, y) } -> std::same_as<T>;
    };

    template<size_t L, HasModFunction T>
    constexpr Vec<L, T> mod(Vec<L, T> const& x, Vec<L, T> const& y)
    {
        return map(x, y, [](T const& xv, T const& yv) { return mod(xv, yv); });
    }

    template<typename T>
    concept HasMinFunction = requires(T x, T y) {
        { min(x, y) } -> std::convertible_to<T>;
    };

    // returns a vector containing `min(a, b)` for each element
    template<size_t L, HasMinFunction T>
    constexpr Vec<L, T> elementwise_min(Vec<L, T> const& x, Vec<L, T> const& y)
    {
        return map(x, y, [](T const& xv, T const& yv) { return min(xv, yv); });
    }

    template<typename T>
    concept HasMaxFunction = requires(T x, T y) {
        { max(x, y) } -> std::convertible_to<T>;
    };

    // returns a vector containing max(a[i], b[i]) for each element
    template<size_t L, HasMaxFunction T>
    constexpr Vec<L, T> elementwise_max(Vec<L, T> const& x, Vec<L, T> const& y)
        requires std::is_arithmetic_v<T>
    {
        return map(x, y, [](T const& xv, T const& yv) { return max(xv, yv); });
    }

    template<typename T>
    concept HasClampFunction = requires(T v, T lo, T hi) {
        { clamp(v, lo, hi) } -> std::convertible_to<T>;
    };

    // clamps each element in `x` between the corresponding elements in `low` and `high`
    template<size_t L, HasClampFunction T>
    constexpr Vec<L, T> clamp(Vec<L, T> const& v, Vec<L, T> const& lo, Vec<L, T> const& hi)
    {
        return map(v, lo, hi, [](T const& vv, T const& lov, T const& hiv) { return clamp(vv, lov, hiv); });
    }

    // clamps each element in `x` between `low` and `high`
    template<size_t L, HasClampFunction T>
    constexpr Vec<L, T> clamp(Vec<L, T> const& v, T const& lo, T const& hi)
    {
        return map(v, [&lo, &hi](T const& vv) { return clamp(vv, lo, hi); });
    }

    // clamps `v` between 0.0 and 1.0
    template<std::floating_point T>
    constexpr T saturate(T num)
    {
        return clamp(num, static_cast<T>(0), static_cast<T>(1));
    }

    // clamps each element in `x` between 0.0 and 1.0
    template<size_t L, std::floating_point T>
    constexpr Vec<L, T> saturate(Vec<L, T> const& x)
    {
        return clamp(x, static_cast<T>(0), static_cast<T>(1));
    }

    // linearly interpolates between `a` and `b` with factor `t`
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

    // linearly interpolates between each element in `x` and `y` with factor `t`
    template<size_t L, typename T, typename Arithmetic>
    constexpr auto lerp(Vec<L, T> const& x, Vec<L, T> const& y, Arithmetic const& t) -> Vec<L, decltype(lerp(x[0], y[0], t))>
        requires std::is_arithmetic_v<T> && std::is_arithmetic_v<Arithmetic>
    {
        return map(x, y, [&t](T const& xv, T const& yv) { return lerp(xv, yv, t); });
    }

    template<size_t L, typename T>
    constexpr Vec<L, bool> elementwise_equal(Vec<L, T> const& x, Vec<L, T> const& y)
    {
        return map(x, y, std::equal_to<T>{});
    }

    template<size_t L, typename T>
    constexpr Vec<L, bool> elementwise_less(Vec<L, T> const& x, Vec<L, T> const& y)
    {
        return map(x, y, std::less<T>{});
    }

    template<size_t L, typename T>
    constexpr Vec<L, bool> elementwise_less(Vec<L, T> const& x, T const& v)
    {
        return map(x, [&v](T const& el) { return el < v; });
    }

    template<size_t L, typename T>
    constexpr Vec<L, bool> elementwise_less_equal(Vec<L, T> const& x, Vec<L, T> const& y)
    {
        return map(x, y, std::less_equal<T>{});
    }

    template<std::floating_point T>
    bool equal_within_absdiff(T x, T y, T absdiff)
    {
        return abs(x - y) < absdiff;
    }

    template<size_t L, std::floating_point T>
    Vec<L, bool> equal_within_absdiff(Vec<L, T> const& x, Vec<L, T> const& y, Vec<L, T> const& absdiff)
    {
        return map(x, y, absdiff, equal_within_absdiff<T>);
    }

    template<size_t L, std::floating_point T>
    Vec<L, bool> equal_within_absdiff(Vec<L, T> const& x, Vec<L, T> const& y, T const& absdiff)
    {
        return elementwise_less(abs(x - y), absdiff);
    }

    template<std::floating_point T>
    bool equal_within_epsilon(T x, T y)
    {
        return equal_within_absdiff(x, y, epsilon_v<T>);
    }

    template<size_t L, std::floating_point T>
    Vec<L, bool> equal_within_epsilon(Vec<L, T> const& x, Vec<L, T> const& y)
    {
        return equal_within_absdiff(x, y, epsilon_v<T>);
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

        T const scaledEpsilon = max(static_cast<T>(1.0), x, y) * epsilon_v<T>;
        return abs(x - y) < scaledEpsilon;
    }

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

    template<size_t L, std::floating_point T>
    Vec<L, bool> equal_within_reldiff(Vec<L, T> const& x, Vec<L, T> const& y, T reldiff)
    {
        return map(x, y, Vec<L, T>(reldiff), equal_within_reldiff<T>);
    }

    template<std::floating_point T>
    bool isnan(T v)
    {
        return std::isnan(v);
    }

    template<size_t L, std::floating_point T>
    Vec<L, bool> isnan(Vec<L, T> const& v)
    {
        return map(v, isnan<T>);
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

    template<typename T>
    constexpr T midpoint(T a, T b)
        requires std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_pointer_v<T>
    {
        return std::midpoint(a, b);
    }

    template<size_t L, typename T>
    Vec<L, T> midpoint(Vec<L, T> const& x, Vec<L, T> const& y)
    {
        return map(x, y, [](T const& xv, T const& yv) { return midpoint(xv, yv); });
    }

    // returns the centroid of all of the provided vectors, or the zero vector if provided no inputs
    template<
        std::ranges::range Range,
        typename VecElement = typename std::ranges::range_value_t<Range>,
        size_t L = std::tuple_size_v<VecElement>,
        typename T = typename VecElement::value_type
    >
    constexpr Vec<L, T> centroid(Range const& r)
        requires std::is_arithmetic_v<T>
    {
        return std::reduce(std::ranges::begin(r), std::ranges::end(r)) / static_cast<T>(std::ranges::size(r));
    }

    template<size_t L, typename T>
    constexpr Vec<L, T> centroid(std::initializer_list<Vec<L, T>> const& vs)
        requires std::is_arithmetic_v<T>
    {
        return centroid(std::span<Vec<L, T> const>{vs});
    }
}
