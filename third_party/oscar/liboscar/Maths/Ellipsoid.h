#pragma once

#include <liboscar/Maths/Quaternion.h>
#include <liboscar/Maths/Vector3.h>

namespace osc
{
    // an ellipsoid located + oriented at an origin position
    struct Ellipsoid final {
        friend bool operator==(const Ellipsoid&, const Ellipsoid&) = default;

        Vector3 origin{};
        Vector3 radii{1.0f};
        Quaternion orientation{};
    };
}
