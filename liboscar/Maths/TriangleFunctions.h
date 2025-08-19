#pragma once

#include <liboscar/Maths/GeometricFunctions.h>
#include <liboscar/Maths/Triangle.h>
#include <liboscar/Maths/Vector3.h>

#include <concepts>
#include <type_traits>

namespace osc
{
    // returns `true` if the given three `Vector3`s could be used to form a triangle
    template<typename T>
    requires std::is_arithmetic_v<T>
    constexpr bool can_form_triangle(const Vec<3, T>& a, const Vec<3, T>& b, const Vec<3, T>& c)
    {
        return a != b and a != c and b != c;  // (this also handles NaNs)
    }

    template<std::floating_point T>
    Vec<3, T> triangle_normal(const Vec<3, T>& a, const Vec<3, T>& b, const Vec<3, T>& c)
    {
        const Vec<3, T> ab = b - a;
        const Vec<3, T> ac = c - a;
        return Vec<3, T>(normalize(cross(ab, ac)));
    }

    inline Vec<3, float> triangle_normal(const Triangle& t)
    {
        return triangle_normal(t.p0, t.p1, t.p2);
    }
}
