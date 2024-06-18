#pragma once

#include <OpenSimCreator/Documents/MeshImporter/MIIDs.h>

#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/Material.h>
#include <oscar/Graphics/MaterialPropertyBlock.h>
#include <oscar/Graphics/Mesh.h>
#include <oscar/Graphics/Scene/SceneDecorationFlags.h>
#include <oscar/Maths/AABB.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/Transform.h>
#include <oscar/Utils/UID.h>

#include <optional>

namespace osc::mi
{
    // something that is being drawn in the scene
    struct DrawableThing final {
        UID id = MIIDs::Empty();
        UID groupId = MIIDs::Empty();
        osc::Mesh mesh{};
        Transform transform{};
        Color color = Color::black();
        SceneDecorationFlags flags = SceneDecorationFlags::None;
        std::optional<Material> material{};
        std::optional<MaterialPropertyBlock> maybePropertyBlock{};
    };

    inline AABB calcBounds(const DrawableThing& dt)
    {
        return transform_aabb(dt.transform, dt.mesh.bounds());
    }
}
