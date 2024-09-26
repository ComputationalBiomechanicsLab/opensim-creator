#include "OpenSimGraphicsHelpers.h"

#include <OpenSimCreator/Documents/Model/IModelStatePair.h>
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
    const ModelRendererParams& renderParams,
    Vec2 viewportDims,
    AntiAliasingLevel antiAliasingLevel,
    float fixupScaleFactor)
{
    SceneRendererParams rv;

    if (viewportDims.x >= 1.0f && viewportDims.y >= 1.0f) {
        rv.dimensions = viewportDims;
    }

    rv.antialiasing_level = antiAliasingLevel;
    rv.light_direction = recommended_light_direction(renderParams.camera);
    renderParams.renderingOptions.applyTo(rv);
    rv.view_matrix = renderParams.camera.view_matrix();
    rv.projection_matrix = renderParams.camera.projection_matrix(aspect_ratio_of(viewportDims));
    rv.near_clipping_plane = renderParams.camera.znear;
    rv.far_clipping_plane = renderParams.camera.zfar;
    rv.view_pos = renderParams.camera.position();
    rv.fixup_scale_factor = fixupScaleFactor;
    rv.light_color = renderParams.lightColor;
    rv.background_color = renderParams.backgroundColor;
    rv.floor_location = renderParams.floorLocation;
    return rv;
}

void osc::GenerateDecorations(
    SceneCache& meshCache,
    const IModelStatePair& msp,
    const OpenSimDecorationOptions& options,
    const std::function<void(const OpenSim::Component&, SceneDecoration&&)>& out)
{
    ComponentAbsPathDecorationTagger pathTagger{};
    ComponentSceneDecorationFlagsTagger flagsTagger{msp.getSelected(), msp.getHovered()};

    auto callback = [pathTagger, flagsTagger, &out](const OpenSim::Component& component, SceneDecoration&& decoration) mutable
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
    const BVH& sceneBVH,
    SceneCache& sceneCache,
    std::span<const SceneDecoration> taggedDrawlist,
    const PolarPerspectiveCamera& camera,
    Vec2 mouseScreenPos,
    const Rect& viewportScreenRect)
{
    OSC_PERF("ModelSceneDecorations/getClosestCollision");

    // un-project 2D mouse cursor into 3D scene as a ray
    const Vec2 mouseRenderPos = mouseScreenPos - viewportScreenRect.p1;
    const Line worldspaceCameraRay = camera.unproject_topleft_pos_to_world_ray(
        mouseRenderPos,
        dimensions_of(viewportScreenRect)
    );

    // find all collisions along the camera ray
    const std::vector<SceneCollision> collisions = get_all_ray_collisions_with_scene(
        sceneBVH,
        sceneCache,
        taggedDrawlist,
        worldspaceCameraRay
    );

    // filter through the collisions list
    const SceneCollision* closestCollision = nullptr;
    for (const SceneCollision& c : collisions)
    {
        if (closestCollision && c.distance_from_ray_origin > closestCollision->distance_from_ray_origin)
        {
            continue;  // it's further away than the current closest collision
        }

        const SceneDecoration& decoration = taggedDrawlist[c.decoration_index];

        if (decoration.id.empty())
        {
            continue;  // filtered out by external filter
        }

        closestCollision = &c;
    }

    return closestCollision ? *closestCollision : std::optional<SceneCollision>{};
}
