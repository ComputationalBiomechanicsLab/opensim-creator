#pragma once

#include <liboscar/maths/vector.h>

namespace osc
{
    struct RayCollision {

        RayCollision() = default;

        RayCollision(float distance, const Vector3& position) :
            distance{distance},
            position{position}
        {}

        float distance{};
        Vector3 position{};
    };
}
