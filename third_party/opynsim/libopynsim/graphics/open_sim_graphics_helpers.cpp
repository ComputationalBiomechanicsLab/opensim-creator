#include "open_sim_graphics_helpers.h"

#include <libopynsim/documents/model/model_state_pair.h>
#include <libopynsim/graphics/component_abs_path_decoration_tagger.h>
#include <libopynsim/graphics/component_scene_decoration_flags_tagger.h>
#include <libopynsim/graphics/model_renderer_params.h>
#include <libopynsim/graphics/open_sim_decoration_generator.h>
#include <liboscar/graphics/anti_aliasing_level.h>
#include <liboscar/graphics/scene/scene_decoration.h>
#include <liboscar/graphics/scene/scene_helpers.h>
#include <liboscar/maths/math_helpers.h>
#include <liboscar/maths/polar_perspective_camera.h>
#include <liboscar/maths/ray.h>
#include <liboscar/maths/rect_functions.h>
#include <liboscar/maths/vector2.h>
#include <liboscar/utilities/perf.h>

#include <algorithm>
#include <optional>
#include <string_view>
#include <vector>

using namespace opyn;

namespace
{
    bool collision_priority_greater(const std::optional<osc::SceneCollision>& lhs, const osc::SceneCollision& rhs)
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

osc::SceneRendererParams opyn::CalcSceneRendererParams(
    const osc::ModelRendererParams& renderParams,
    osc::Vector2 viewportDims,
    float viewportDevicePixelRatio,
    osc::AntiAliasingLevel antiAliasingLevel,
    float fixupScaleFactor)
{
    osc::SceneRendererParams rv;

    if (viewportDims.x() >= 1.0f && viewportDims.y() >= 1.0f) {
        rv.dimensions = viewportDims;
    }
    rv.device_pixel_ratio = viewportDevicePixelRatio;
    rv.anti_aliasing_level = antiAliasingLevel;
    rv.light_direction = recommended_light_direction(renderParams.camera);
    renderParams.renderingOptions.applyTo(rv);
    rv.view_matrix = renderParams.camera.view_matrix();
    rv.projection_matrix = renderParams.camera.projection_matrix(aspect_ratio_of(viewportDims));
    rv.near_clipping_plane = renderParams.camera.znear;
    rv.far_clipping_plane = renderParams.camera.zfar;
    rv.viewer_position = renderParams.camera.position();
    rv.fixup_scale_factor = fixupScaleFactor;
    rv.light_color = renderParams.lightColor;
    rv.background_color = renderParams.backgroundColor;
    rv.floor_position = renderParams.floorLocation;
    return rv;
}

void opyn::GenerateDecorations(
    osc::SceneCache& meshCache,
    const osc::ModelStatePair& msp,
    const OpenSimDecorationOptions& options,
    const std::function<void(const OpenSim::Component&, osc::SceneDecoration&&)>& out)
{
    osc::ComponentAbsPathDecorationTagger pathTagger{};
    osc::ComponentSceneDecorationFlagsTagger flagsTagger{msp.getSelected(), msp.getHovered()};

    auto callback = [pathTagger, flagsTagger, &out](
        const OpenSim::Component& component,
        osc::SceneDecoration&& decoration) mutable
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

std::optional<osc::SceneCollision> opyn::GetClosestCollision(
    const osc::BVH& sceneBVH,
    osc::SceneCache& sceneCache,
    std::span<const osc::SceneDecoration> taggedDrawlist,
    const osc::PolarPerspectiveCamera& camera,
    osc::Vector2 mouseScreenPosition,
    const osc::Rect& viewportScreenRect)
{
    OSC_PERF("osc::GetClosestCollision");

    // un-project 2D mouse cursor into 3D scene as a ray
    const osc::Vector2 mouseRenderPosition = mouseScreenPosition - viewportScreenRect.ypd_top_left();
    const osc::Ray worldSpaceCameraRay = camera.unproject_topleft_position_to_world_ray(
        mouseRenderPosition,
        viewportScreenRect.dimensions()
    );

    // iterate over all collisions along the camera ray and find the best one
    std::optional<osc::SceneCollision> best;
    for_each_ray_collision_with_scene(
        sceneBVH,
        sceneCache,
        taggedDrawlist,
        worldSpaceCameraRay,
        [&best](osc::SceneCollision&& sceneCollision)
        {
            if (not sceneCollision.decoration_id.empty()
                and collision_priority_greater(best, sceneCollision)) {

                best = std::move(sceneCollision);
            }
        });
    return best;
}
