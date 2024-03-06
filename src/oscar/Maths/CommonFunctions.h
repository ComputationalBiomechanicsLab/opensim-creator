#pragma once

#include <oscar/Maths/Angle.h>
#include <oscar/Maths/Constants.h>
#include <oscar/Maths/Functors.h>
#include <oscar/Maths/Vec.h>

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
    T abs(T v)
    {
        return std::abs(v);
    }

    template<std::floating_point T>
    T abs(T v)
    {
        return std::fabs(v);
    }

    template<size_t L, typename T>
    Vec<L, T> abs(Vec<L, T> const& v)
        requires std::is_arithmetic_v<T>
    {
        return map(v, abs<T>);
    }

    template<std::floating_point T>
    T floor(T num)
    {
        return std::floor(num);
    }

    template<size_t L, std::floating_point T>
    Vec<L, T> floor(Vec<L, T> const& v)
    {
        return map(v, floor<T>);
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

    template<
        std::floating_point Rep1,
        AngularUnitTraits Units1,
        std::floating_point Rep2,
        AngularUnitTraits Units2
    >
    auto mod(Angle<Rep1, Units1> x, Angle<Rep2, Units2> y) -> std::common_type_t<decltype(x), decltype(y)>
    {
        using CA = std::common_type_t<decltype(x), decltype(y)>;
        return CA{mod(CA{x}.count(), CA{y}.count())};
    }

    template<size_t L, typename T>
    constexpr Vec<L, T> mod(Vec<L, T> const& a, Vec<L, T> const& b)
        requires std::is_arithmetic_v<T>
    {
        return map(a, b, mod<T>);
    }

    // returns the smallest of `a` and `b`
    template<typename GenType>
    constexpr GenType min(GenType a, GenType b)
        requires std::is_arithmetic_v<GenType>
    {
        return std::min(a, b);
    }

    // returns the smallest of `a` and `b`, accounting for differences in angular units (e.g. 180_deg < 1_turn)
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

    // returns the smallest element in `v`
    template<size_t L, typename T>
    constexpr T min(Vec<L, T> const& v)
        requires std::is_arithmetic_v<T>
    {
        auto it = v.begin();
        auto const end = v.end();
        T rv = *it;
        while (++it != end) {
            if (*it < rv) {
                rv = *it;
            }
        }
        return rv;
    }

    // returns an iterator to the smallest element in `v`
    template<size_t L, typename T>
    constexpr typename Vec<L, T>::const_iterator min_element(Vec<L, T> const& v)
        requires std::is_arithmetic_v<T>
    {
        return std::min_element(v.begin(), v.end());
    }

    // returns an iterator to the smallest element in `v`
    template<size_t L, typename T>
    constexpr typename Vec<L, T>::iterator min_element(Vec<L, T>& v)
        requires std::is_arithmetic_v<T>
    {
        return std::min_element(v.begin, v.end());
    }

    // returns the index of the smallest element in `v`
    template<size_t L, typename T>
    constexpr typename Vec<L, T>::size_type min_element_index(Vec<L, T> const& v)
        requires std::is_arithmetic_v<T>
    {
        return std::distance(v.begin(), min_element(v));
    }

    // returns a vector containing `min(a, b)` for each element
    template<size_t L, typename T>
    constexpr Vec<L, T> elementwise_min(Vec<L, T> const& a, Vec<L, T> const& b)
        requires std::is_arithmetic_v<T>
    {
        return map(a, b, min<T>);
    }

    // returns the largest of `a` and `b`
    template<typename GenType>
    constexpr GenType max(GenType a, GenType b)
        requires std::is_arithmetic_v<GenType>
    {
        return std::max(a, b);
    }

    // returns the largest of `a` and `b`, accounting for differences in angular units (e.g. 180_deg < 1_turn)
    template<
        std::floating_point Rep1,
        AngularUnitTraits Units1,
        std::floating_point Rep2,
        AngularUnitTraits Units2
    >
    constexpr auto max(Angle<Rep1, Units1> x, Angle<Rep2, Units2> y) -> std::common_type_t<decltype(x), decltype(y)>
    {
        using CA = std::common_type_t<decltype(x), decltype(y)>;
        return CA{max(CA{x}.count(), CA{y}.count())};
    }

    // returns the largest element in `v`
    template<size_t L, typename T>
    constexpr T max(Vec<L, T> const& v)
        requires std::is_arithmetic_v<T>
    {
        auto it = v.begin();
        auto const end = v.end();
        T rv = *it;
        while (++it != end) {
            if (*it > rv) {
                rv = *it;
            }
        }
        return rv;
    }

    // returns an iterator to the largest element in `v`
    template<size_t L, typename T>
    constexpr typename Vec<L, T>::const_iterator max_element(Vec<L, T> const& v)
        requires std::is_arithmetic_v<T>
    {
        return std::max_element(v.begin(), v.end());
    }

    // returns an iterator to the largest element in `v`
    template<size_t L, typename T>
    constexpr typename Vec<L, T>::iterator max_element(Vec<L, T>& v)
        requires std::is_arithmetic_v<T>
    {
        return std::max_element(v.begin, v.end());
    }

    // returns the index of the largest element in `v`
    template<size_t L, typename T>
    constexpr typename Vec<L, T>::size_type max_element_index(Vec<L, T> const& v)
        requires std::is_arithmetic_v<T>
    {
        return std::distance(v.begin(), max_element(v));
    }

    // returns a vector containing max(a[i], b[i]) for each element
    template<size_t L, typename T>
    constexpr Vec<L, T> elementwise_max(Vec<L, T> const& a, Vec<L, T> const& b)
        requires std::is_arithmetic_v<T>
    {
        return map(a, b, max<T>);
    }

    // clamps `v` between `low` and `high` (inclusive)
    template<typename GenType>
    constexpr GenType clamp(GenType v, GenType low, GenType high)
        requires std::is_arithmetic_v<GenType>
    {
        return std::clamp(v, low, high);
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

    // clamps each element in `x` between the corresponding elements in `low` and `high`
    template<size_t L, typename T>
    constexpr Vec<L, T> clamp(Vec<L, T> const& v, Vec<L, T> const& low, Vec<L, T> const& high)
        requires std::is_arithmetic_v<T>
    {
        return map(v, low, high, clamp<T>);
    }

    // clamps each element in `x` between `low` and `high`
    template<size_t L, typename T>
    constexpr Vec<L, T> clamp(Vec<L, T> const& v, T const& low, T const& high)
        requires std::is_arithmetic_v<T>
    {
        return map(v, [&low, &high](T const& el) { return clamp(el, low, high); });
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
    constexpr GenType lerp(GenType const& x, GenType const& y, UInterpolant const& a)
        requires std::is_arithmetic_v<GenType> && std::is_floating_point_v<UInterpolant>
    {
        return static_cast<GenType>(static_cast<UInterpolant>(x) * (static_cast<UInterpolant>(1) - a) + static_cast<UInterpolant>(y) * a);
    }

    // linearly interpolates between each element in `x` and `y` with factor `a`
    template<size_t L, typename T, typename UInterpolant>
    constexpr Vec<L, T> lerp(Vec<L, T> const& x, Vec<L, T> const& y, UInterpolant const& a)
        requires std::is_arithmetic_v<T> && std::is_floating_point_v<UInterpolant>
    {
        return Vec<L, T>(Vec<L, UInterpolant>(x) * (static_cast<UInterpolant>(1) - a) + Vec<L, UInterpolant>(y) * a);
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

    template<size_t L, typename T>
    constexpr bool lexicographical_compare(Vec<L, T> const& x, Vec<L, T> const& y)
    {
        return std::lexicographical_compare(x.begin(), x.end(), y.begin(), y.end());
    }

    template<std::floating_point T>
    bool equal_within_absdiff(T x, T y, T epsilon_v)
    {
        return abs(x - y) < epsilon_v;
    }

    template<size_t L, std::floating_point T>
    Vec<L, bool> equal_within_absdiff(Vec<L, T> const& x, Vec<L, T> const& y, Vec<L, T> const& eps)
    {
        return map(x, y, eps, equal_within_absdiff<T>);
    }

    template<size_t L, std::floating_point T>
    Vec<L, bool> equal_within_absdiff(Vec<L, T> const& x, Vec<L, T> const& y, T const& eps)
    {
        return elementwise_less(abs(x - y), eps);
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

        T const scaledEpsilon = std::max(static_cast<T>(1.0), x, y) * epsilon_v<T>;
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
        return map(x, y, Vec<L, T>(releps), equal_within_reldiff<T>);
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
        requires std::is_arithmetic_v<T>
    {
        return std::midpoint(a, b);
    }

    template<size_t L, typename T>
    Vec<L, T> midpoint(Vec<L, T> const& a, Vec<L, T> const& b)
    {
        return map(a, b, midpoint<T>);
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
        using std::begin;
        using std::end;
        using std::size;

        return std::reduce(begin(r), end(r)) / static_cast<T>(size(r));
    }

    template<size_t L, typename T>
    constexpr Vec<L, T> centroid(std::initializer_list<Vec<L, T>> const& vs)
        requires std::is_arithmetic_v<T>
    {
        return centroid(std::span<Vec<L, T> const>{vs});
    }
}
