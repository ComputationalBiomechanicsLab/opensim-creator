#pragma once

#include <oscar/Maths/UnitVec.h>
#include <oscar/Maths/Vec.h>

#include <cmath>
#include <concepts>
#include <cstddef>
#include <type_traits>

namespace osc
{
    template<std::floating_point T>
    T sqrt(T v)
    {
        return std::sqrt(v);
    }

    template<std::floating_point T>
    constexpr T dot(T a, T b)
    {
        return a * b;
    }

    template<typename T>
    constexpr T dot(Vec<2, T> const& a, Vec<2, T> const& b)
        requires std::is_arithmetic_v<T>
    {
        Vec<2, T> tmp(a * b);
        return tmp.x + tmp.y;
    }

    template<typename T>
    constexpr T dot(Vec<3, T> const& a, Vec<3, T> const& b)
        requires std::is_arithmetic_v<T>
    {
        Vec<3, T> tmp(a * b);
        return tmp.x + tmp.y + tmp.z;
    }

    template<typename T>
    constexpr T dot(Vec<4, T> const& a, Vec<4, T> const& b)
        requires std::is_arithmetic_v<T>
    {
        Vec<4, T> tmp(a * b);
        return tmp.x + tmp.y + tmp.z + tmp.w;
    }

    // calculates the cross product of the two vectors
    template<typename T>
    constexpr Vec<3, T> cross(Vec<3, T> const& x, Vec<3, T> const& y)
        requires std::is_arithmetic_v<T>
    {
        return Vec<3, T>(
            x.y * y.z - y.y * x.z,
            x.z * y.x - y.z * x.x,
            x.x * y.y - y.x * x.y
        );
    }

    template<std::floating_point T>
    constexpr UnitVec<3, T> cross(UnitVec<3, T> const& x, UnitVec<3, T> const& y)
        requires std::is_arithmetic_v<T>
    {
        return UnitVec<3, T>::AlreadyNormalized(
            x[1] * y[2] - y[1] * x[2],
            x[2] * y[0] - y[2] * x[0],
            x[0] * y[1] - y[0] * x[1]
        );
    }

    template<std::floating_point T>
    T inversesqrt(T x)
    {
        return static_cast<T>(1) / sqrt(x);
    }

    template<size_t L, std::floating_point T>
    Vec<L, T> normalize(Vec<L, T> const& v)
    {
        return v * inversesqrt(dot(v, v));
    }

    // returns the length of the provided vector
    template<size_t L, std::floating_point T>
    float length(Vec<L, T> const& v)
    {
        return sqrt(dot(v, v));
    }

    // returns the squared length of the provided vector
    template<size_t L, std::floating_point T>
    constexpr float length2(Vec<L, T> const& v)
    {
        return dot(v, v);
    }
}
