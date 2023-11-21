#pragma once

#include <oscar/Maths/Vec3.hpp>

namespace osc
{
    struct RayCollision {
        RayCollision() = default;
        RayCollision(float distance_, Vec3 const& position_) :
            distance{distance_},
            position{position_}
        {
        }

        float distance{};
        Vec3 position{};
    };
}
