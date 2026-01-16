#pragma once

#include <liboscar/maths/ellipsoid.h>
#include <liboscar/maths/vector3.h>

#include <array>

namespace osc
{
    // returns the direction of each axis of `ellipsoid`
    constexpr std::array<Vector3, 3> axis_directions_of(const Ellipsoid& ellipsoid)
    {
        return {
            ellipsoid.orientation * Vector3{1.0f, 0.0f, 0.0f},
            ellipsoid.orientation * Vector3{0.0f, 1.0f, 0.0f},
            ellipsoid.orientation * Vector3{0.0f, 0.0f, 1.0f},
        };
    }
}
