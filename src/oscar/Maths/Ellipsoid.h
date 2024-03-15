#pragma once

#include <oscar/Maths/Quat.h>
#include <oscar/Maths/Vec3.h>

namespace osc
{
    // an ellipsoid located + oriented at an origin location
    struct Ellipsoid final {
        Vec3 origin{};
        Vec3 radii{1.0f};
        Quat orientation{};
    };
}
