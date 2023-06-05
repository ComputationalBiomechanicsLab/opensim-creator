#include "ModelSceneDecorations.hpp"

#include <oscar/Graphics/GraphicsHelpers.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/PolarPerspectiveCamera.hpp>
#include <oscar/Utils/Perf.hpp>

osc::ModelSceneDecorations::ModelSceneDecorations() = default;

void osc::ModelSceneDecorations::clear()
{
    m_Drawlist.clear();
    m_BVH.clear();
}

void osc::ModelSceneDecorations::reserve(size_t n)
{
    m_Drawlist.reserve(n);
}

void osc::ModelSceneDecorations::computeBVH()
{
    UpdateSceneBVH(m_Drawlist, m_BVH);
}

std::optional<osc::AABB> osc::ModelSceneDecorations::getRootAABB() const
{
    return m_BVH.getRootAABB();
}

std::optional<osc::SceneCollision> osc::ModelSceneDecorations::getClosestCollision(
    PolarPerspectiveCamera const& camera,
    glm::vec2 mouseScreenPos,
    Rect const& viewportScreenRect) const
{
    OSC_PERF("ModelSceneDecorations/getClosestCollision");

    // un-project 2D mouse cursor into 3D scene as a ray
    glm::vec2 const mouseRenderPos = mouseScreenPos - viewportScreenRect.p1;
    Line const worldspaceCameraRay = camera.unprojectTopLeftPosToWorldRay(
        mouseRenderPos,
        Dimensions(viewportScreenRect)
    );

    // find all collisions along the camera ray
    std::vector<SceneCollision> collisions = GetAllSceneCollisions(
        m_BVH,
        m_Drawlist,
        worldspaceCameraRay
    );

    // filter through the collisions list
    SceneCollision const* closestCollision = nullptr;
    for (SceneCollision const& c : collisions)
    {
        if (closestCollision && c.distanceFromRayOrigin > closestCollision->distanceFromRayOrigin)
        {
            continue;  // it's further away than the current closest collision
        }

        SceneDecoration const& decoration = m_Drawlist[c.decorationIndex];

        if (decoration.id.empty())
        {
            continue;  // filtered out by external filter
        }

        closestCollision = &c;
    }

    return closestCollision ? *closestCollision : std::optional<SceneCollision>{};
}