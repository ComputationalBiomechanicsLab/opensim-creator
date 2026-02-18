#pragma once

#include <libopensimcreator/Documents/MeshImporter/MiIDs.h>

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
        UID id = MiIDs::Empty();
        UID groupId = MiIDs::Empty();
        osc::Mesh mesh{};
        Transform transform{};
        SceneDecorationShading shading = Color::black();
        SceneDecorationFlags flags = SceneDecorationFlag::Default;
    };

    inline std::optional<AABB> calcBounds(const DrawableThing& dt)
    {
        return dt.mesh.bounds().transform([&dt](const AABB& localBounds)
        {
            return transform_aabb(dt.transform, localBounds);
        });
    }
}
