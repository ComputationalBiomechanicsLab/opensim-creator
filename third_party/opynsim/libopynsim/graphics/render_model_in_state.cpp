#include "render_model_in_state.h"

#include <libopynsim/graphics/open_sim_decoration_options.h>
#include <libopynsim/graphics/open_sim_decoration_generator.h>
#include <libopynsim/model.h>
#include <libopynsim/model_state.h>

#include <liboscar/graphics/scene/scene_cache.h>
#include <liboscar/graphics/scene/scene_renderer.h>
#include <liboscar/graphics/scene/scene_renderer_params.h>
#include <liboscar/graphics/graphics.h>
#include <liboscar/maths/aabb_functions.h>
#include <liboscar/maths/polar_perspective_camera.h>
#include <liboscar/platform/app.h>
#include <liboscar/utilities/assertions.h>

#include <optional>
#include <utility>
#include <vector>

osc::Texture2D opyn::render_model_in_state(
    const Model& model,
    const ModelState& model_state,
    std::pair<int, int> dimensions,
    bool zoom_to_fit)
{
    OSC_ASSERT_ALWAYS(dimensions.first > 0 and dimensions.second > 0 && "The dimensions of a render must be positive integers");

    // Initialize application state
    osc::App app;

    // Generate 3D scene
    osc::SceneCache scene_cache;
    const OpenSimDecorationOptions decoration_options;
    const std::vector<osc::SceneDecoration> decorations = GenerateModelDecorations(
        scene_cache,
        model.opensim_model(),
        model_state.simbody_state(),
        decoration_options
    );

    // Setup scene camera
    osc::PolarPerspectiveCamera camera;
    const osc::Vector2 dimensions_vec{dimensions.first, dimensions.second};

    // Handle initial autofocus
    if (std::exchange(zoom_to_fit, false)) {
        if (const auto aabb = osc::bounding_aabb_of(decorations, &osc::SceneDecoration::world_space_bounds)) {
            osc::auto_focus(camera, *aabb, osc::aspect_ratio_of(osc::App::get().main_window_dimensions()));
        }
    }

    // Use camera to render scene to `RenderTexture` (GPU)
    osc::SceneRenderer scene_renderer{scene_cache};
    osc::SceneRendererParams scene_renderer_params = {
        .dimensions = dimensions_vec,
        .anti_aliasing_level = osc::AntiAliasingLevel{4},
        .view_matrix = camera.view_matrix(),
        .projection_matrix = camera.projection_matrix(osc::aspect_ratio_of(dimensions_vec)),
    };
    scene_renderer.render(decorations, scene_renderer_params);
    const osc::RenderTexture& rendered_scene = scene_renderer.upd_render_texture();

    // Blit `RenderTexture` to `Texture2D` (CPU accessible, for Python)
    osc::Texture2D rv{dimensions_vec};
    osc::graphics::copy_texture(rendered_scene, rv);
    return rv;
}
