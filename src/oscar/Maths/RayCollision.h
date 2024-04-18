#pragma once

#include <oscar/Maths/Vec3.h>

namespace osc
{
    struct RayCollision {
        RayCollision() = default;

        RayCollision(float distance_, const Vec3& position_) :
            distance{distance_},
            position{position_}
        {}

        float distance{};
        Vec3 position{};
    };
}
