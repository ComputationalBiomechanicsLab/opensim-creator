#pragma once

#include <oscar/Maths/Quat.h>
#include <oscar/Maths/Vec3.h>

namespace osc
{
    struct Ellipsoid final {
        Vec3 origin{};
        Vec3 radii{1.0f};
        Quat orientation{};
    };
}
