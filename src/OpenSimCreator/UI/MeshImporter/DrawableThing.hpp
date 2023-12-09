#pragma once

#include <OpenSimCreator/Documents/MeshImporter/MIIDs.hpp>

#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/Material.hpp>
#include <oscar/Graphics/MaterialPropertyBlock.hpp>
#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Maths/AABB.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Scene/SceneDecorationFlags.hpp>
#include <oscar/Utils/UID.hpp>

#include <optional>

namespace osc::mi
{
    // something that is being drawn in the scene
    struct DrawableThing final {
        UID id = MIIDs::Empty();
        UID groupId = MIIDs::Empty();
        osc::Mesh mesh;
        Transform transform;
        Color color = Color::black();
        SceneDecorationFlags flags = SceneDecorationFlags::None;
        std::optional<Material> maybeMaterial;
        std::optional<MaterialPropertyBlock> maybePropertyBlock;
    };

    inline AABB calcBounds(DrawableThing const& dt)
    {
        return osc::TransformAABB(dt.mesh.getBounds(), dt.transform);
    }
}
