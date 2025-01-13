#pragma once

#include <libOpenSimCreator/Documents/MeshImporter/MIIDs.h>

#include <liboscar/Graphics/Color.h>
#include <liboscar/Graphics/Mesh.h>
#include <liboscar/Graphics/Scene/SceneDecorationFlags.h>
#include <liboscar/Graphics/Scene/SceneDecorationShading.h>
#include <liboscar/Maths/AABB.h>
#include <liboscar/Maths/MathHelpers.h>
#include <liboscar/Maths/Transform.h>
#include <liboscar/Utils/UID.h>

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

    inline AABB calcBounds(const DrawableThing& dt)
    {
        return transform_aabb(dt.transform, dt.mesh.bounds());
    }
}
