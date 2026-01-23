#include "model_viewer.h"

#include <libopynsim/graphics/open_sim_decoration_generator.h>
#include <libopynsim/model.h>
#include <libopynsim/model_state.h>

#include <liboscar/graphics/scene/scene_cache.h>
#include <liboscar/graphics/scene/scene_decoration.h>
#include <liboscar/graphics/scene/scene_renderer.h>
#include <liboscar/graphics/scene/scene_renderer_params.h>
#include <liboscar/maths/polar_perspective_camera.h>
#include <liboscar/maths/vector2.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/app_metadata.h>
#include <liboscar/platform/widget.h>
#include <liboscar/ui/oscimgui.h>

#include <vector>

#include "liboscar/graphics/graphics.h"

using namespace opyn;

namespace
{
    std::vector<osc::SceneDecoration> generate_scene(
        osc::SceneCache& scene_cache,
        const Model& model,
        const ModelState& model_state)
    {
        osc::OpenSimDecorationOptions options;
        return osc::GenerateModelDecorations(
            scene_cache,
            model.opensim_model(),
            model_state.simbody_state(),
            options
        );
    }

    class BasicModelViewer final : public osc::Widget {
    public:
        explicit BasicModelViewer(const Model& model, const ModelState& model_state) :
            decorations_{generate_scene(scene_cache_, model, model_state)}
        {}
    private:
        bool impl_on_event(osc::Event& e) override { return ui_context_.on_event(e); }

        void impl_on_draw() final
        {
            osc::App::upd().clear_main_window();
            ui_context_.on_start_new_frame();

            // Update the scene camera state based on the user's inputs.
            osc::ui::update_polar_camera_from_all_inputs(
                camera,
                osc::Rect::from_origin_and_dimensions({}, osc::App::get().main_window_pixel_dimensions()),
                std::nullopt
            );

            const osc::Vector2 dimensions = osc::App::get().main_window_dimensions();

            osc::SceneRendererParams scene_renderer_params{
                .dimensions = dimensions,
                .device_pixel_ratio = osc::App::get().main_window_device_pixel_ratio(),
                .anti_aliasing_level = osc::App::get().anti_aliasing_level(),
                .view_matrix = camera.view_matrix(),
                .projection_matrix = camera.projection_matrix(osc::aspect_ratio_of(dimensions))
            };
            scene_renderer_.render(decorations_, scene_renderer_params);
            ui_context_.render();

            osc::graphics::blit_to_main_window(scene_renderer_.upd_render_texture());
        }

        osc::ui::Context ui_context_{osc::App::upd()};
        osc::SceneCache scene_cache_;
        osc::SceneRenderer scene_renderer_{scene_cache_};
        std::vector<osc::SceneDecoration> decorations_;
        osc::PolarPerspectiveCamera camera;
    };
}


void opyn::view_model_in_state(const Model& model, const ModelState& state)
{
    osc::App::main<BasicModelViewer>(osc::AppMetadata{}, model, state);
}
