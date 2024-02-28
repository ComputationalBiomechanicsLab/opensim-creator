#include "OpenSimGraphicsHelpers.h"

#include <OpenSimCreator/Documents/Model/IConstModelStatePair.h>
#include <OpenSimCreator/Graphics/ComponentAbsPathDecorationTagger.h>
#include <OpenSimCreator/Graphics/ComponentSceneDecorationFlagsTagger.h>
#include <OpenSimCreator/Graphics/ModelRendererParams.h>
#include <OpenSimCreator/Graphics/OpenSimDecorationGenerator.h>

#include <oscar/Graphics/AntiAliasingLevel.h>
#include <oscar/Graphics/Scene/SceneDecoration.h>
#include <oscar/Graphics/Scene/SceneHelpers.h>
#include <oscar/Maths/Line.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/PolarPerspectiveCamera.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Utils/Perf.h>

#include <optional>
#include <vector>

using namespace osc;

SceneRendererParams osc::CalcSceneRendererParams(
    ModelRendererParams const& renderParams,
    Vec2 viewportDims,
    AntiAliasingLevel antiAliasingLevel,
    float fixupScaleFactor)
{
    SceneRendererParams params;
    if (viewportDims.x >= 1.0f && viewportDims.y >= 1.0f)
    {
        params.dimensions = viewportDims;
    }
    params.antiAliasingLevel = antiAliasingLevel;
    params.lightDirection = RecommendedLightDirection(renderParams.camera);
    params.drawFloor = renderParams.renderingOptions.getDrawFloor();
    params.viewMatrix = renderParams.camera.view_matrix();
    params.projectionMatrix = renderParams.camera.projection_matrix(AspectRatio(viewportDims));
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
    SceneCache& meshCache,
    IConstModelStatePair const& msp,
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

std::optional<SceneCollision> osc::GetClosestCollision(
    BVH const& sceneBVH,
    SceneCache& sceneCache,
    std::span<SceneDecoration const> taggedDrawlist,
    PolarPerspectiveCamera const& camera,
    Vec2 mouseScreenPos,
    Rect const& viewportScreenRect)
{
    OSC_PERF("ModelSceneDecorations/getClosestCollision");

    // un-project 2D mouse cursor into 3D scene as a ray
    Vec2 const mouseRenderPos = mouseScreenPos - viewportScreenRect.p1;
    Line const worldspaceCameraRay = camera.unprojectTopLeftPosToWorldRay(
        mouseRenderPos,
        dimensions(viewportScreenRect)
    );

    // find all collisions along the camera ray
    std::vector<SceneCollision> const collisions = GetAllSceneCollisions(
        sceneBVH,
        sceneCache,
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
