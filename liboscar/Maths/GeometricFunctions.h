#pragma once

#include <liboscar/Maths/Vec.h>

#include <cmath>
#include <concepts>
#include <cstddef>
#include <type_traits>

namespace osc
{
    // returns the square root of `num`
    template<std::floating_point T>
    T sqrt(T num)
    {
        return std::sqrt(num);
    }

    // returns the inverse square root of `x` (i.e. `1/sqrt(x)`)
    template<std::floating_point T>
    T inversesqrt(T x)
    {
        return static_cast<T>(1) / sqrt(x);
    }

    // returns the dot product of `x` and `y` (i.e. `x * y`)
    template<typename T>
    requires std::is_arithmetic_v<T>
    constexpr T dot(T x, T y)
    {
        return x * y;
    }

    // returns the dot product of `x` and `y`
    template<size_t L, typename T>
    requires std::is_arithmetic_v<T>
    constexpr T dot(const Vec<L, T>& x, const Vec<L, T>& y)
    {
        T acc = x[0] * y[0];
        for (size_t i = 1; i < L; ++i) {
            acc += x[i] * y[i];
        }
        return acc;
    }

    // returns the cross product of `x` and `y`
    template<typename T>
    requires std::is_arithmetic_v<T>
    constexpr Vec<3, T> cross(const Vec<3, T>& x, const Vec<3, T>& y)
    {
        return Vec<3, T>(
            x.y * y.z - y.y * x.z,
            x.z * y.x - y.z * x.x,
            x.x * y.y - y.x * x.y
        );
    }

    // returns the length of the provided vector
    template<size_t L, std::floating_point T>
    float length(const Vec<L, T>& v)
    {
        return sqrt(dot(v, v));
    }

    // returns the squared length of the provided vector
    template<size_t L, std::floating_point T>
    constexpr float length2(const Vec<L, T>& v)
    {
        return dot(v, v);
    }

    // returns `v` normalized to a length of 1
    template<size_t L, std::floating_point T>
    Vec<L, T> normalize(const Vec<L, T>& v)
    {
        return v * inversesqrt(dot(v, v));
    }

    // returns the aspect ratio of the vector (effectively: `FloatingPointResult{x}/FloatingPointResult{y}`)
    template<std::integral T, std::floating_point FloatingPointResult = float>
    constexpr FloatingPointResult aspect_ratio_of(Vec<2, T> v)
    {
        return static_cast<FloatingPointResult>(v.x) / static_cast<FloatingPointResult>(v.y);
    }

    // returns the aspect ratio of `v` (effectively: `v.x/v.y`)
    template<std::floating_point T>
    constexpr T aspect_ratio_of(Vec<2, T> v)
    {
        return v.x / v.y;
    }

    // returns the area of a 2D rectangle that begins at the origin and ends at `v`
    template<typename T>
    requires std::is_arithmetic_v<T>
    constexpr T area_of(const Vec<2, T>& v)
    {
        return v.x * v.y;
    }
}
