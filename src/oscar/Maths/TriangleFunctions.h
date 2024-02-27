#pragma once

#include <oscar/Maths/GeometricFunctions.h>
#include <oscar/Maths/Triangle.h>
#include <oscar/Maths/UnitVec3.h>
#include <oscar/Maths/Vec3.h>

#include <concepts>
#include <type_traits>

namespace osc
{
    // returns `true` if the given three Vec3s could be used to form a triangle
    template<typename T>
    constexpr bool can_form_triangle(Vec<3, T> const& a, Vec<3, T> const& b, Vec<3, T> const& c)
        requires std::is_arithmetic_v<T>
    {
        return a != b && a != c && b != c;  // (this also handles NaNs)
    }

    template<std::floating_point T>
    UnitVec<3, T> triangle_normal(Vec<3, T> const& a, Vec<3, T> const& b, Vec<3, T> const& c)
    {
        Vec<3, T> const ab = b - a;
        Vec<3, T> const ac = c - a;
        return UnitVec<3, T>(cross(ab, ac));
    }

    inline UnitVec<3, float> triangle_normal(Triangle const& t)
    {
        return triangle_normal(t.p0, t.p1, t.p2);
    }
}
