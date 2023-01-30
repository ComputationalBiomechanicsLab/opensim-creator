#pragma once

#include <glm/vec3.hpp>
#include <nonstd/span.hpp>

#include <cstddef>
#include <string>
#include <utility>

namespace osc
{
    // describes a collision between a ray and a decoration in the scene
    class SceneCollision final {
    public:
        SceneCollision(
            std::string decorationID_,
            size_t decorationIndex_,
            glm::vec3 const& worldspaceLocation_,
            float distanceFromRayOrigin_) :

            decorationID{std::move(decorationID_)},
            worldspaceLocation{worldspaceLocation_},
            decorationIndex{decorationIndex_},
            distanceFromRayOrigin{distanceFromRayOrigin_}
        {
        }

        std::string decorationID;
        size_t decorationIndex;
        glm::vec3 worldspaceLocation;
        float distanceFromRayOrigin;
    };
}