#include "render_model_in_state.h"

#include <libopynsim/graphics/open_sim_decoration_options.h>
#include <libopynsim/graphics/open_sim_decoration_generator.h>
#include <libopynsim/platform/opynsim_app.h>
#include <libopynsim/model.h>
#include <libopynsim/model_state.h>

#include <liboscar/graphics/scene/scene_cache.h>
#include <liboscar/graphics/scene/scene_renderer.h>
#include <liboscar/graphics/scene/scene_renderer_params.h>
#include <liboscar/graphics/camera_v2.h>
#include <liboscar/graphics/graphics.h>
#include <liboscar/maths/aabb_functions.h>
#include <liboscar/maths/polar_perspective_camera.h>
#include <liboscar/utilities/assertions.h>

#include <optional>
#include <utility>
#include <vector>

osc::Texture2D opyn::render_model_in_state(
    OPynSimApp&,
    const Model& model,
    const ModelState& model_state,
    osc::Vector2 dimensions,
    osc::Color background_color,
    bool draw_floor,
    osc::SceneCache* scene_cache,
    const osc::CameraV2* camera)
{
    OSC_ASSERT_ALWAYS(dimensions.x() > 0 and dimensions.y() > 0 && "The dimensions of a render must be positive integers");

    // Create local scene cache if caller did not provide one.
    std::optional<osc::SceneCache> local_cache;
    if (not scene_cache) {
        scene_cache = &local_cache.emplace();
    }

    // Generate decorations
    const std::vector<osc::SceneDecoration> decorations = model.decorations(*scene_cache, model_state);
    const auto aspect_ratio = osc::aspect_ratio_of(dimensions);

    // Create local camera if caller did not provide one.
    std::optional<osc::CameraV2> local_camera;
    if (not camera) {
        osc::PolarPerspectiveCamera polar_camera;
        polar_camera.phi = {};
        polar_camera.theta = {};

        // Handle auto-framing
        if (const auto aabb = osc::bounding_aabb_of(decorations, &osc::SceneDecoration::world_space_bounds)) {
            osc::auto_focus(polar_camera, *aabb, aspect_ratio);
        }

        auto& lc = local_camera.emplace();
        lc.set_view_matrix_override(polar_camera.view_matrix());
        lc.set_projection_matrix_override(polar_camera.projection_matrix(aspect_ratio));
        camera = &lc;
    }

    // Use camera to render scene to `RenderTexture` (GPU)
    osc::SceneRenderer scene_renderer{*scene_cache};
    osc::SceneRendererParams scene_renderer_params = {
        .dimensions = dimensions,
        .anti_aliasing_level = osc::AntiAliasingLevel{4},
        .draw_floor = draw_floor,
        .view_matrix = camera->view_matrix(),
        .projection_matrix = camera->projection_matrix(osc::aspect_ratio_of(dimensions)),
        .background_color = background_color,
    };
    scene_renderer.render(decorations, scene_renderer_params);
    const osc::RenderTexture& rendered_scene = scene_renderer.upd_render_texture();

    // Blit `RenderTexture` to `Texture2D` (CPU accessible, for Python)
    osc::Texture2D rv{dimensions};
    osc::graphics::copy_texture(rendered_scene, rv);
    return rv;
}
