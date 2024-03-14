#pragma once

#include <oscar/Maths/Vec2.h>

namespace osc
{
    // a circle in 2D space
    //
    // - see `Disc` for the 3D equivalent
    struct Circle final {
        Vec2 origin{};
        float radius = 1.0f;
    };
}
