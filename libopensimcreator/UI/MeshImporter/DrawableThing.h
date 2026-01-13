#pragma once

#include <libopensimcreator/Documents/MeshImporter/MIIDs.h>

#include <liboscar/graphics/Color.h>
#include <liboscar/graphics/Mesh.h>
#include <liboscar/graphics/scene/SceneDecorationFlags.h>
#include <liboscar/graphics/scene/SceneDecorationShading.h>
#include <liboscar/maths/AABB.h>
#include <liboscar/maths/AABBFunctions.h>
#include <liboscar/maths/Transform.h>
#include <liboscar/utils/UID.h>

namespace osc::mi
{
    // something that is being drawn in the scene
    struct DrawableThing final {
        UID id = MIIDs::Empty();
        UID groupId = MIIDs::Empty();
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
