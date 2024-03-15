#pragma once

#include <oscar/Maths/Vec2.h>

namespace osc
{
    // a circle positioned at an origin location
    //
    // - see `Disc` for the 3D equivalent of this
    struct Circle final {
        Vec2 origin{};
        float radius = 1.0f;
    };
}
