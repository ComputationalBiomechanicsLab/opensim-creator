#pragma once

#include <oscar/Maths/Ellipsoid.h>
#include <oscar/Maths/Vec3.h>

#include <array>

namespace osc
{
    // returns the direction of each axis of `ellipsoid`
    constexpr std::array<Vec3, 3> axis_directions_of(const Ellipsoid& ellipsoid)
    {
        return {
            ellipsoid.orientation * Vec3{1.0f, 0.0f, 0.0f},
            ellipsoid.orientation * Vec3{0.0f, 1.0f, 0.0f},
            ellipsoid.orientation * Vec3{0.0f, 0.0f, 1.0f},
        };
    }
}
