#include "OpenSimGraphicsHelpers.h"

#include <libopensimcreator/Documents/Model/IModelStatePair.h>
#include <libopensimcreator/Graphics/ComponentAbsPathDecorationTagger.h>
#include <libopensimcreator/Graphics/ComponentSceneDecorationFlagsTagger.h>
#include <libopensimcreator/Graphics/ModelRendererParams.h>
#include <libopensimcreator/Graphics/OpenSimDecorationGenerator.h>

#include <liboscar/Graphics/AntiAliasingLevel.h>
#include <liboscar/Graphics/Scene/SceneDecoration.h>
#include <liboscar/Graphics/Scene/SceneHelpers.h>
#include <liboscar/Maths/MathHelpers.h>
#include <liboscar/Maths/PolarPerspectiveCamera.h>
#include <liboscar/Maths/Ray.h>
#include <liboscar/Maths/RectFunctions.h>
#include <liboscar/Maths/Vec2.h>
#include <liboscar/Utils/Perf.h>

#include <algorithm>
#include <optional>
#include <string_view>
#include <vector>

using namespace osc;

namespace
{
    bool collision_priority_greater(const std::optional<SceneCollision>& lhs, const SceneCollision& rhs)
    {
        if (not lhs) {
            return true;  // any collision is better than no collision
        }
        // if a collision has an ID (presumed to be an absolute path) that is prefixed by the other
        // then it's a subcomponent, which should be prioritized for hit-testing (#592).
        if (lhs->decoration_id.size() < rhs.decoration_id.size()
            and rhs.decoration_id.starts_with(lhs->decoration_id)) {

            return true;
        }
        if (lhs->decoration_id.size() > rhs.decoration_id.size()
            and lhs->decoration_id.starts_with(rhs.decoration_id)) {

            return false;
        }
        // else: the closest collision gets priority
        return rhs.world_distance_from_ray_origin < lhs->world_distance_from_ray_origin;
    }
}

SceneRendererParams osc::CalcSceneRendererParams(
    const ModelRendererParams& renderParams,
    Vec2 viewportDims,
    float viewportDevicePixelRatio,
    AntiAliasingLevel antiAliasingLevel,
    float fixupScaleFactor)
{
    SceneRendererParams rv;

    if (viewportDims.x >= 1.0f && viewportDims.y >= 1.0f) {
        rv.dimensions = viewportDims;
    }
    rv.device_pixel_ratio = viewportDevicePixelRatio;
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
    OSC_PERF("osc::GetClosestCollision");

    // un-project 2D mouse cursor into 3D scene as a ray
    const Vec2 mouseRenderPos = mouseScreenPos - viewportScreenRect.ypd_top_left();
    const Ray worldSpaceCameraRay = camera.unproject_topleft_pos_to_world_ray(
        mouseRenderPos,
        viewportScreenRect.dimensions()
    );

    // iterate over all collisions along the camera ray and find the best one
    std::optional<SceneCollision> best;
    for_each_ray_collision_with_scene(
        sceneBVH,
        sceneCache,
        taggedDrawlist,
        worldSpaceCameraRay,
        [&best](SceneCollision&& sceneCollision)
        {
            if (not sceneCollision.decoration_id.empty()
                and collision_priority_greater(best, sceneCollision)) {

                best = std::move(sceneCollision);
            }
        });
    return best;
}
