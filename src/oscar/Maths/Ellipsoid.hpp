#pragma once

#include <oscar/Maths/Vec3.hpp>

#include <array>

namespace osc
{
    struct Ellipsoid final {
        Vec3 origin = {};
        Vec3 radii = {1.0f, 1.0f, 1.0f};
        std::array<Vec3, 3> radiiDirections =
        {
            Vec3{1.0f, 0.0f, 0.0f},
            Vec3{0.0f, 1.0f, 0.0f},
            Vec3{0.0f, 0.0f, 1.0f},
        };
    };
}
