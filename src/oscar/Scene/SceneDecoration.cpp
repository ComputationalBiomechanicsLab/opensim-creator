#include "SceneDecoration.hpp"

#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Maths/AABB.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Scene/SceneDecoration.hpp>

bool osc::operator==(SceneDecoration const& lhs, SceneDecoration const& rhs) noexcept
{
    return
        lhs.mesh == rhs.mesh &&
        lhs.transform == rhs.transform &&
        lhs.color == rhs.color &&
        lhs.id == rhs.id &&
        lhs.flags == rhs.flags &&
        lhs.maybeMaterial == rhs.maybeMaterial &&
        lhs.maybeMaterialProps == rhs.maybeMaterialProps;
}
