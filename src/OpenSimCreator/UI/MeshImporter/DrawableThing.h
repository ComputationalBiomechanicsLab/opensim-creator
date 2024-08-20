#pragma once

#include <OpenSimCreator/Documents/MeshImporter/MIIDs.h>

#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/Mesh.h>
#include <oscar/Graphics/Scene/SceneDecorationFlags.h>
#include <oscar/Graphics/Scene/SceneDecorationShading.h>
#include <oscar/Maths/AABB.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/Transform.h>
#include <oscar/Utils/UID.h>

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
