#pragma once

#include <oscar/Maths/Quat.h>
#include <oscar/Maths/Vec3.h>

namespace osc
{
    // a positioned + oriented ellipsoid in 3D space
    struct Ellipsoid final {
        Vec3 origin{};
        Vec3 radii{1.0f};
        Quat orientation{};
    };
}
