#pragma once

#include <liboscar/Maths/Vector3.h>

namespace osc
{
    struct RayCollision {

        RayCollision() = default;

        RayCollision(float distance_, const Vector3& position_) :
            distance{distance_},
            position{position_}
        {}

        float distance{};
        Vector3 position{};
    };
}
