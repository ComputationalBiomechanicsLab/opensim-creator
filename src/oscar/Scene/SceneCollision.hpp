#pragma once

#include <oscar/Maths/Vec3.hpp>

#include <cstddef>
#include <limits>
#include <string>
#include <utility>

namespace osc
{
    // describes a collision between a ray and a decoration in the scene
    struct SceneCollision final {
        std::string decorationID;
        size_t decorationIndex = 0;
        Vec3 worldspaceLocation{};
        float distanceFromRayOrigin = std::numeric_limits<float>::max();
    };
}
