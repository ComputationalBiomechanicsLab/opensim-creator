#pragma once

#include <liboscar/maths/geometric_functions.h>
#include <liboscar/maths/triangle.h>
#include <liboscar/maths/vector3.h>

#include <concepts>
#include <type_traits>

namespace osc
{
    // returns `true` if the given three `Vector3`s could be used to form a triangle
    template<typename T>
    requires std::is_arithmetic_v<T>
    constexpr bool can_form_triangle(const Vector<T, 3>& a, const Vector<T, 3>& b, const Vector<T, 3>& c)
    {
        return a != b and a != c and b != c;  // (this also handles NaNs)
    }

    template<std::floating_point T>
    Vector<T, 3> triangle_normal(const Vector<T, 3>& a, const Vector<T, 3>& b, const Vector<T, 3>& c)
    {
        const Vector<T, 3> ab = b - a;
        const Vector<T, 3> ac = c - a;
        return Vector<T, 3>(normalize(cross(ab, ac)));
    }

    inline Vector<float, 3> triangle_normal(const Triangle& t)
    {
        return triangle_normal(t.p0, t.p1, t.p2);
    }
}
