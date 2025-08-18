#pragma once

#include <liboscar/Maths/Quat.h>
#include <liboscar/Maths/Vec3.h>

namespace osc
{
    // an ellipsoid located + oriented at an origin position
    struct Ellipsoid final {
        friend bool operator==(const Ellipsoid&, const Ellipsoid&) = default;

        Vec3 origin{};
        Vec3 radii{1.0f};
        Quat orientation{};
    };
}
