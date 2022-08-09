#include "SceneDecoration.hpp"

#include "src/Graphics/Renderer.hpp"
#include "src/Graphics/SceneDecoration.hpp"
#include "src/Maths/AABB.hpp"
#include "src/Maths/Geometry.hpp"
#include "src/Maths/Transform.hpp"

osc::AABB osc::GetWorldspaceAABB(SceneDecoration const& cd)
{
    return TransformAABB(cd.mesh->getBounds(), cd.transform);
}
