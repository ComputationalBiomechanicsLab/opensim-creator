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

    // returns a vector containing `abs(v)` for each `v` in `x`
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

    // returns a vector containing `floor(v)` for each `v` in `x`
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

    // satisfied if `mod(x, y)` is a valid expression for type `T`
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

    // satisfied if `min(x, y)` is a valid expression for type `T`
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

    // satisfied if `max(x, y)` is a valid expression for type `T`
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

    // returns `clamp(num, 0, 1)`
    template<std::floating_point T>
    constexpr T saturate(T num)
    {
        return clamp(num, static_cast<T>(0), static_cast<T>(1));
    }

    // returns a vector containing `clamp(xv, 0, 1)` for each `xv` in `x`
    template<size_t L, std::floating_point T>
    constexpr Vec<L, T> saturate(Vec<L, T> const& x)
    {
        return clamp(x, static_cast<T>(0), static_cast<T>(1));
    }

    // returns the equivalent of `a + t(b - a)`
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

    // returns a vector containing `lerp(xv, yv, t)` for each `(xv, yv)` in `x` and `y`
    template<size_t L, typename T, typename Arithmetic>
    constexpr auto lerp(Vec<L, T> const& x, Vec<L, T> const& y, Arithmetic const& t) -> Vec<L, decltype(lerp(x[0], y[0], t))>
        requires std::is_arithmetic_v<T> && std::is_arithmetic_v<Arithmetic>
    {
        return map(x, y, [&t](T const& xv, T const& yv) { return lerp(xv, yv, t); });
    }

    // returns a vector containing `xv == yv` for each `(xv, yv)` in `x` and `y`
    template<size_t L, typename T>
    constexpr Vec<L, bool> elementwise_equal(Vec<L, T> const& x, Vec<L, T> const& y)
    {
        return map(x, y, std::equal_to<T>{});
    }

    // returns a vector containing `xv < yv` for each `(xv, yv)` in `x` and `y`
    template<size_t L, typename T>
    constexpr Vec<L, bool> elementwise_less(Vec<L, T> const& x, Vec<L, T> const& y)
    {
        return map(x, y, std::less<T>{});
    }

    // returns a vector containing `xv < v` for each `xv` in `x`
    template<size_t L, typename T>
    constexpr Vec<L, bool> elementwise_less(Vec<L, T> const& x, T const& v)
    {
        return map(x, [&v](T const& el) { return el < v; });
    }

    // returns a vector containing `xv <= yv` for each `(xv, yv)` in `x` and `y`
    template<size_t L, typename T>
    constexpr Vec<L, bool> elementwise_less_equal(Vec<L, T> const& x, Vec<L, T> const& y)
    {
        return map(x, y, std::less_equal<T>{});
    }

    // returns true if the absolute difference between `x` and `y` is less than `absdiff`
    template<std::floating_point T>
    bool equal_within_absdiff(T x, T y, T absdiff)
    {
        return abs(x - y) < absdiff;
    }

    // returns a vector containing `equal_within_absdiff(xv, yv, absdiffv)` for each `(xv, yv, absdiffv)` in `x`, `y`, and `absdiff`
    template<size_t L, std::floating_point T>
    Vec<L, bool> equal_within_absdiff(Vec<L, T> const& x, Vec<L, T> const& y, Vec<L, T> const& absdiff)
    {
        return map(x, y, absdiff, equal_within_absdiff<T>);
    }

    // returns a vector containing `equal_within_absdiff(xv, yv, absdiff)` for each `(xv, yv)` in `x` and `y`
    template<size_t L, std::floating_point T>
    Vec<L, bool> equal_within_absdiff(Vec<L, T> const& x, Vec<L, T> const& y, T const& absdiff)
    {
        return elementwise_less(abs(x - y), absdiff);
    }

    // returns true if the absolute difference between `x` and `y` is less than epsilon (machine precision)
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

    // returns true if the absolute difference between `x` and `y` is less than epsilon (machine precision) after
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

    // returns true if the relative difference between `x` and `y` is less than `reldiff` (fraction)
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
        auto begin = std::ranges::begin(r);
        auto end = std::ranges::end(r);

        if (begin == end) {
            return Vec<L, T>{};  // edge-case: no vectors provided
        }

        if constexpr (std::is_same_v<T, float>) {
            // floats: use `double` as the aggregator
            //
            // unlikely to affect perf because the memory bandwith is the same. This side-steps
            // precision issues when aggregating large collections of points
            auto adder = [](Vec<L, double> const& init, Vec<L, T> const& v) { return init + Vec<L, double>{v}; };
            return Vec<L, T>{std::reduce(begin, end, Vec<L, double>{}, adder) / static_cast<double>(std::ranges::size(r))};
        }
        else {
            // else: compute + return the arithmetic mean with no conversion
            return std::reduce(begin, end) / static_cast<T>(std::ranges::size(r));
        }
    }

    template<size_t L, typename T>
    constexpr Vec<L, T> centroid(std::initializer_list<Vec<L, T>> const& vs)
        requires std::is_arithmetic_v<T>
    {
        return centroid(std::span<Vec<L, T> const>{vs});
    }
}
