#pragma once

#include <libopensimcreator/documents/mesh_importer/mi_ids.h>

#include <liboscar/graphics/color.h>
#include <liboscar/graphics/mesh.h>
#include <liboscar/graphics/scene/scene_decoration_flags.h>
#include <liboscar/graphics/scene/scene_decoration_shading.h>
#include <liboscar/maths/aabb.h>
#include <liboscar/maths/aabb_functions.h>
#include <liboscar/maths/transform.h>
#include <liboscar/utilities/uid.h>

namespace osc
{
    // something that is being drawn in the scene
    struct DrawableThing final {
        UID id = MiIDs::empty();
        UID group_id = MiIDs::empty();
        Mesh mesh;
        Transform transform{};
        SceneDecorationShading shading = Color::black();
        SceneDecorationFlags flags = SceneDecorationFlag::Default;
    };

    inline std::optional<AABB> calc_bounds(const DrawableThing& dt)
    {
        return dt.mesh.bounds().transform(
            [&dt](const AABB& local_bounds) { return transform_aabb(dt.transform, local_bounds); });
    }
}
