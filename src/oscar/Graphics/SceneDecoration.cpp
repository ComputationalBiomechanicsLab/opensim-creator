#include "SceneDecoration.hpp"

#include "oscar/Graphics/Color.hpp"
#include "oscar/Graphics/Mesh.hpp"
#include "oscar/Graphics/SceneDecoration.hpp"
#include "oscar/Maths/AABB.hpp"
#include "oscar/Maths/MathHelpers.hpp"
#include "oscar/Maths/Transform.hpp"

bool osc::operator==(SceneDecoration const& a, SceneDecoration const& b) noexcept
{
    return
        a.mesh == b.mesh &&
        a.transform == b.transform &&
        a.color == b.color &&
        a.id == b.id &&
        a.flags == b.flags &&
        a.maybeMaterial == b.maybeMaterial &&
        a.maybeMaterialProps == b.maybeMaterialProps;
}
