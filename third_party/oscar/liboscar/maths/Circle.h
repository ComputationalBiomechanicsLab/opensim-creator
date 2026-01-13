#pragma once

#include <liboscar/maths/Vector2.h>

namespace osc
{
    // a circle positioned at an origin position
    //
    // - see `Disc` for the 3D equivalent of this
    struct Circle final {
        friend bool operator==(const Circle&, const Circle&) = default;

        Circle expanded_by(float amount) const { return Circle{origin, radius + amount}; }

        Vector2 origin{};
        float radius = 1.0f;
    };
}
