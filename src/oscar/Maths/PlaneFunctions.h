#pragma once

#include <oscar/Maths/AABB.h>
#include <oscar/Maths/AABBFunctions.h>
#include <oscar/Maths/AnalyticPlane.h>
#include <oscar/Maths/CommonFunctions.h>
#include <oscar/Maths/GeometricFunctions.h>
#include <oscar/Maths/Plane.h>
#include <oscar/Maths/Vec3.h>

namespace osc
{
    // returns an `AnalyticPlane` converted from a point on a plane's surface, plus the plane's normal direction
    constexpr AnalyticPlane to_analytic_plane(const Vec3& point, const Vec3& normal)
    {
        return AnalyticPlane{.distance = dot(point, normal), .normal = normal };
    }

    // returns an `AnalyticPlane` converted from a (point-normal form) `Plane`
    constexpr AnalyticPlane to_analytic_plane(const Plane& plane)
    {
        return AnalyticPlane{.distance = dot(plane.origin, plane.normal), .normal = plane.normal};
    }

    // returns the signed distance between the (normal-oriented) surface of `plane` and `vec`
    constexpr float signed_distance_between(const AnalyticPlane& plane, const Vec3& vec)
    {
        return dot(vec, plane.normal) - plane.distance;
    }

    // returns the signed distance between the (normal-oriented) surface of `plane` and `vec`
    constexpr float signed_distance_between(const Plane& plane, const Vec3& vec)
    {
        return dot(vec, plane.normal) - dot(plane.origin, plane.normal);
    }

    // tests if `aabb` is entirely in front of `plane`
    inline bool is_in_front_of(const AnalyticPlane& plane, const AABB& aabb)
    {
        // originally found in: https://learnopengl.com/Guest-Articles/2021/Scene/Frustum-Culling
        // which was based on : https://gdbooks.gitbooks.io/3dcollisions/content/Chapter2/static_aabb_plane.html
        const float r = dot(half_widths_of(aabb), abs(plane.normal));
        return signed_distance_between(plane, centroid_of(aabb)) > r;
    }

    // tests if `aabb` is entirely in front of `plane`
    inline bool is_in_front_of(const Plane& plane, const AABB& aabb)
    {
        return is_in_front_of(to_analytic_plane(plane), aabb);
    }
}
