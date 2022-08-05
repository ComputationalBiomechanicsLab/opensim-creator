#include "SceneDecorationNew.hpp"

#include "src/Graphics/Mesh.hpp"
#include "src/Graphics/SceneDecorationNew.hpp"
#include "src/Maths/AABB.hpp"
#include "src/Maths/Geometry.hpp"
#include "src/Maths/Transform.hpp"

osc::AABB osc::GetWorldspaceAABB(SceneDecorationNew const& cd)
{
    return TransformAABB(cd.mesh->getAABB(), cd.transform);
}
