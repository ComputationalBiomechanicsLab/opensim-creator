#include "scene_decoration.h"

#include <liboscar/maths/aabb_functions.h>

using namespace osc;

std::optional<AABB> osc::SceneDecoration::world_space_bounds() const
{
    return mesh.bounds().transform([this](const AABB& local_aabb)
    {
        return transform_aabb(transform, local_aabb);
    });
}
