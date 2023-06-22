#include "OpenSimGraphicsHelpers.hpp"

#include "OpenSimCreator/Graphics/ModelRendererParams.hpp"
#include "OpenSimCreator/Graphics/ComponentAbsPathDecorationTagger.hpp"
#include "OpenSimCreator/Graphics/ComponentSceneDecorationFlagsTagger.hpp"
#include "OpenSimCreator/Graphics/OpenSimDecorationGenerator.hpp"
#include "OpenSimCreator/Model/VirtualConstModelStatePair.hpp"

#include <oscar/Graphics/GraphicsHelpers.hpp>
#include <oscar/Graphics/SceneDecoration.hpp>
#include <oscar/Maths/Line.hpp>
#include <oscar/Maths/PolarPerspectiveCamera.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Utils/Perf.hpp>

#include <glm/vec2.hpp>

#include <optional>
#include <vector>

osc::SceneRendererParams osc::CalcSceneRendererParams(
    ModelRendererParams const& renderParams,
    glm::vec2 viewportDims,
    int32_t samples,
    float fixupScaleFactor)
{
    SceneRendererParams params;
    if (viewportDims.x >= 1.0f && viewportDims.y >= 1.0f)
    {
        params.dimensions = viewportDims;
    }
    params.samples = samples;
    params.lightDirection = RecommendedLightDirection(renderParams.camera);
    params.drawFloor = renderParams.renderingOptions.getDrawFloor();
    params.viewMatrix = renderParams.camera.getViewMtx();
    params.projectionMatrix = renderParams.camera.getProjMtx(osc::AspectRatio(viewportDims));
    params.nearClippingPlane = renderParams.camera.znear;
    params.farClippingPlane = renderParams.camera.zfar;
    params.viewPos = renderParams.camera.getPos();
    params.fixupScaleFactor = fixupScaleFactor;
    params.drawRims = renderParams.renderingOptions.getDrawSelectionRims();
    params.drawMeshNormals = renderParams.renderingOptions.getDrawMeshNormals();
    params.drawShadows = renderParams.renderingOptions.getDrawShadows();
    params.lightColor = renderParams.lightColor;
    params.backgroundColor = renderParams.backgroundColor;
    params.floorLocation = renderParams.floorLocation;
    return params;
}

void osc::GenerateDecorations(
    MeshCache& meshCache,
    VirtualConstModelStatePair const& msp,
    OpenSimDecorationOptions const& options,
    std::function<void(OpenSim::Component const&, SceneDecoration&&)> const& out)
{
    ComponentAbsPathDecorationTagger pathTagger{};
    ComponentSceneDecorationFlagsTagger flagsTagger{msp.getSelected(), msp.getHovered()};

    auto callback = [pathTagger, flagsTagger, &out](OpenSim::Component const& component, SceneDecoration&& decoration) mutable
    {
        pathTagger(component, decoration);
        flagsTagger(component, decoration);
        out(component, std::move(decoration));
    };

    GenerateModelDecorations(
        meshCache,
        msp.getModel(),
        msp.getState(),
        options,
        msp.getFixupScaleFactor(),
        callback
    );
}

std::optional<osc::SceneCollision> osc::GetClosestCollision(
    BVH const& sceneBVH,
    nonstd::span<SceneDecoration const> taggedDrawlist,
    PolarPerspectiveCamera const& camera,
    glm::vec2 mouseScreenPos,
    Rect const& viewportScreenRect)
{
    OSC_PERF("ModelSceneDecorations/getClosestCollision");

    // un-project 2D mouse cursor into 3D scene as a ray
    glm::vec2 const mouseRenderPos = mouseScreenPos - viewportScreenRect.p1;
    Line const worldspaceCameraRay = camera.unprojectTopLeftPosToWorldRay(
        mouseRenderPos,
        Dimensions(viewportScreenRect)
    );

    // find all collisions along the camera ray
    std::vector<SceneCollision> const collisions = GetAllSceneCollisions(
        sceneBVH,
        taggedDrawlist,
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

        SceneDecoration const& decoration = taggedDrawlist[c.decorationIndex];

        if (decoration.id.empty())
        {
            continue;  // filtered out by external filter
        }

        closestCollision = &c;
    }

    return closestCollision ? *closestCollision : std::optional<SceneCollision>{};
}