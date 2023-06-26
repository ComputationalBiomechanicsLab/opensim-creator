#pragma once

#include <glm/vec3.hpp>

namespace osc
{
    struct RayCollision {

        RayCollision(
            float distance_,
            glm::vec3 position_) :

            distance{distance_},
            position{position_}
        {
        }

        float distance;
        glm::vec3 position;
    };
}
