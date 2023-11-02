#pragma once

#include <oscar/Maths/Vec3.hpp>

#include <nonstd/span.hpp>

#include <cstddef>
#include <string>
#include <utility>

namespace osc
{
    // describes a collision between a ray and a decoration in the scene
    struct SceneCollision final {

        SceneCollision(
            std::string decorationID_,
            size_t decorationIndex_,
            Vec3 const& worldspaceLocation_,
            float distanceFromRayOrigin_) :

            decorationID{std::move(decorationID_)},
            decorationIndex{decorationIndex_},
            worldspaceLocation{worldspaceLocation_},
            distanceFromRayOrigin{distanceFromRayOrigin_}
        {
        }

        std::string decorationID;
        size_t decorationIndex;
        Vec3 worldspaceLocation;
        float distanceFromRayOrigin;
    };
}
