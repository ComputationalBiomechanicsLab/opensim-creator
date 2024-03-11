#pragma once

#include <oscar/Maths/Ellipsoid.h>
#include <oscar/Maths/Vec3.h>

#include <array>

namespace osc
{
    // returns the direction of each radii of the ellipsoid
    constexpr std::array<Vec3, 3> radii_directions(Ellipsoid const& e)
    {
        return {
            e.orientation * Vec3{1.0f, 0.0f, 0.0f},
            e.orientation * Vec3{0.0f, 1.0f, 0.0f},
            e.orientation * Vec3{0.0f, 0.0f, 1.0f},
        };
    }
}
