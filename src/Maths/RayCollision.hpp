#pragma once

#include <glm/vec3.hpp>

namespace osc
{
    struct RayCollision final {
        float distance;
        glm::vec3 position;

        RayCollision() = default;
        RayCollision(float distance_, glm::vec3 position_) :
            distance{distance_},
            position{position_}
        {
        }
    };
}
