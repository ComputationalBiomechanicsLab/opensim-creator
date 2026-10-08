#include "open_sim_graphics_helpers.h"

#include <libopynsim/documents/model/model_state_pair.h>
#include <libopynsim/graphics/component_abs_path_decoration_tagger.h>
#include <libopynsim/graphics/component_scene_decoration_flags_tagger.h>
#include <libopynsim/graphics/model_renderer_params.h>
#include <libopynsim/graphics/open_sim_decoration_generator.h>
#include <liboscar/graphics/anti_aliasing_level.h>
#include <liboscar/graphics/camera.h>
#include <liboscar/graphics/scene/scene_decoration.h>
#include <liboscar/graphics/scene/scene_helpers.h>
#include <liboscar/maths/math_helpers.h>
#include <liboscar/maths/ray.h>
#include <liboscar/maths/rect_functions.h>
#include <liboscar/maths/vector.h>
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

osc::SceneRendererParams opyn::calc_scene_renderer_params(
    const ModelRendererParams& render_params,
    osc::Vector2 viewport_dims,
    float viewport_device_pixel_ratio,
    osc::AntiAliasingLevel anti_aliasing_level,
    float fixup_scale_factor)
{
    osc::SceneRendererParams rv;

    if (viewport_dims.x() >= 1.0f && viewport_dims.y() >= 1.0f) {
        rv.dimensions = viewport_dims;
    }
    rv.device_pixel_ratio = viewport_device_pixel_ratio;
    rv.anti_aliasing_level = anti_aliasing_level;
    rv.light_direction = recommended_light_direction(render_params.camera);
    render_params.rendering_options.apply_to(rv);
    rv.view_matrix = render_params.camera.view_matrix();
    rv.projection_matrix = render_params.camera.projection_matrix(aspect_ratio_of(viewport_dims));
    rv.near_clipping_plane = render_params.camera.near_clipping_plane();
    rv.far_clipping_plane = render_params.camera.far_clipping_plane();
    rv.viewer_position = render_params.camera.position();
    rv.fixup_scale_factor = fixup_scale_factor;
    rv.light_color = render_params.light_color;
    rv.background_color = render_params.background_color;
    rv.floor_position = render_params.floor_location;
    return rv;
}

void opyn::generate_decorations(
    osc::SceneCache& mesh_cache,
    const opyn::ModelStatePair& msp,
    const OpenSimDecorationOptions& options,
    const std::function<void(const OpenSim::Component&, osc::SceneDecoration&&)>& out)
{
    ComponentAbsPathDecorationTagger path_tagger{};
    ComponentSceneDecorationFlagsTagger flags_tagger{msp.get_selected(), msp.get_hovered()};

    auto callback = [path_tagger, flags_tagger, &out](
        const OpenSim::Component& component,
        osc::SceneDecoration&& decoration) mutable
    {
        path_tagger(component, decoration);
        flags_tagger(component, decoration);
        out(component, std::move(decoration));
    };

    generate_model_decorations(
        mesh_cache,
        msp.get_model(),
        msp.get_state(),
        options,
        msp.get_fixup_scale_factor(),
        callback
    );
}

std::optional<osc::SceneCollision> opyn::get_closest_collision(
    const osc::BVH& scene_bvh,
    osc::SceneCache& scene_cache,
    std::span<const osc::SceneDecoration> tagged_drawlist,
    const osc::Camera& camera,
    osc::Vector2 mouse_screen_position,
    const osc::Rect& viewport_screen_rect)
{
    OSC_PERF("osc::GetClosestCollision");

    // un-project 2D mouse cursor into 3D scene as a ray
    const osc::Ray world_space_camera_ray = camera.ui_to_world(
        mouse_screen_position,
        viewport_screen_rect
    );

    // iterate over all collisions along the camera ray and find the best one
    std::optional<osc::SceneCollision> best;
    for_each_ray_collision_with_scene(
        scene_bvh,
        scene_cache,
        tagged_drawlist,
        world_space_camera_ray,
        [&best](osc::SceneCollision&& scene_collision)
        {
            if (not scene_collision.decoration_id.empty()
                and collision_priority_greater(best, scene_collision)) {

                best = std::move(scene_collision);
            }
        });
    return best;
}
