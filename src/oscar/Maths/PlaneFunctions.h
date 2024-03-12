#pragma once

#include <oscar/Maths/AABB.h>
#include <oscar/Maths/AABBFunctions.h>
#include <oscar/Maths/CommonFunctions.h>
#include <oscar/Maths/GeometricFunctions.h>
#include <oscar/Maths/Plane.h>
#include <oscar/Maths/Vec3.h>

namespace osc
{
    // returns the signed distance between the (normal-oriented) surface of `plane` and `v`
    constexpr float signed_distance_between(Plane const& plane, Vec3 const& v)
    {
        return dot(v, plane.normal) - dot(plane.origin, plane.normal);
    }

    // tests if `aabb` is entirely in front of `plane`
    inline bool is_in_front_of(Plane const& plane, AABB const& aabb)
    {
        // originally found in: https://learnopengl.com/Guest-Articles/2021/Scene/Frustum-Culling
        // which was based on : https://gdbooks.gitbooks.io/3dcollisions/content/Chapter2/static_aabb_plane.html

        float const r = dot(half_widths(aabb), abs(plane.normal));
        return signed_distance_between(plane, centroid(aabb)) > r;
    }
}
