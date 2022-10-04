#pragma once

#include <glm/vec3.hpp>
#include <nonstd/span.hpp>

#include <cstddef>

namespace osc
{
    // describes a collision between a ray and a decoration in the scene
    struct SceneCollision final {
        glm::vec3 worldspaceLocation;
        size_t decorationIndex;
        float distanceFromRayOrigin;

        SceneCollision(
            glm::vec3 const& worldspaceLocation_,
            size_t decorationIndex_,
            float distanceFromRayOrigin_) :
            worldspaceLocation{worldspaceLocation_},
            decorationIndex{decorationIndex_},
            distanceFromRayOrigin{distanceFromRayOrigin_}
        {
        }
    };
}