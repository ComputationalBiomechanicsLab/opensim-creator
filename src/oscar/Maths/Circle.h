#pragma once

#include <oscar/Maths/Vec2.h>

namespace osc
{
    // a circle positioned at an origin location
    //
    // - see `Disc` for the 3D equivalent of this
    struct Circle final {
        friend bool operator==(const Circle&, const Circle&) = default;

        Vec2 origin{};
        float radius = 1.0f;
    };
}
