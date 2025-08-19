#pragma once

#include <liboscar/Maths/Vector3.h>
#include <liboscar/Utils/StringName.h>

#include <cstddef>
#include <limits>

namespace osc
{
    // describes a collision between a ray and a decoration in the scene
    struct SceneCollision final {
        StringName decoration_id{};
        size_t decoration_index = 0;
        Vector3 world_position{};
        float world_distance_from_ray_origin = std::numeric_limits<float>::max();
    };
}
