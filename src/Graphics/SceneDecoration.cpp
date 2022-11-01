#include "SceneDecoration.hpp"

#include "src/Graphics/Mesh.hpp"
#include "src/Graphics/SceneDecoration.hpp"
#include "src/Maths/AABB.hpp"
#include "src/Maths/MathHelpers.hpp"
#include "src/Maths/Transform.hpp"

osc::AABB osc::GetWorldspaceAABB(SceneDecoration const& cd)
{
    return TransformAABB(cd.mesh->getBounds(), cd.transform);
}

bool osc::operator==(SceneDecoration const& a, SceneDecoration const& b) noexcept
{
    return
        *a.mesh == *b.mesh &&
        a.transform == b.transform &&
        a.color == b.color &&
        a.id == b.id &&
        a.flags == b.flags &&
        a.maybeMaterial == b.maybeMaterial &&
        a.maybeMaterialProps == b.maybeMaterialProps;
}