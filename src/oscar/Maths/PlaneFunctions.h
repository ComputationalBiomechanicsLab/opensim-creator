#pragma once

#include <oscar/Maths/GeometricFunctions.h>
#include <oscar/Maths/Plane.h>
#include <oscar/Maths/Vec3.h>

namespace osc
{
    constexpr float signed_distance_between(Plane const& plane, Vec3 const& v)
    {
        return dot(v, plane.normal) - dot(plane.origin, plane.normal);
    }
}
