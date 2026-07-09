#include "show_model_in_state.h"

#include <libopynsim/graphics/open_sim_decoration_generator.h>
#include <libopynsim/platform/opynsim_app.h>
#include <libopynsim/ui/ui_callbacks.h>
#include <libopynsim/model.h>
#include <libopynsim/model_state.h>

#include <liboscar/graphics/scene/scene_cache.h>
#include <liboscar/graphics/scene/scene_decoration.h>
#include <liboscar/graphics/scene/scene_renderer.h>
#include <liboscar/graphics/scene/scene_renderer_params.h>
#include <liboscar/graphics/camera_v2.h>
#include <liboscar/graphics/graphics.h>
#include <liboscar/maths/aabb.h>
#include <liboscar/maths/aabb_functions.h>
#include <liboscar/maths/polar_perspective_camera.h>
#include <liboscar/maths/vector.h>
#include <liboscar/platform/widget.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/utilities/scope_exit.h>

#include <optional>
#include <utility>
#include <vector>

using namespace opyn;

namespace
{
    class BasicModelViewer final : public osc::Widget {
    public:
        explicit BasicModelViewer(
            const Model& model,
            const ModelState& model_state,
            osc::Color background_color,
            bool draw_floor,
            osc::SceneCache* scene_cache,
            UiCallbacks callbacks) :

            callbacks_{std::move(callbacks)},
            local_scene_cache_{scene_cache ? std::optional<osc::SceneCache>{} : osc::SceneCache{}},
            scene_cache_{scene_cache ? scene_cache : &local_scene_cache_.value()},
            decorations_{model.decorations(*scene_cache_, model_state)},
            background_color_{background_color},
            fit_camera_on_next_frame_{true},
            draw_floor_{draw_floor}
        {}
    private:
        bool impl_on_event(osc::Event& e) override
        {
            return ui_context_.on_event(e);
        }

        void impl_on_tick() override
        {
            callbacks_.on_tick_begin();
        }

        void impl_on_draw() final
        {
            osc::App::upd().main_window_clear();
            ui_context_.on_start_new_frame();

            // Handle initial autofocus
            if (std::exchange(fit_camera_on_next_frame_, false)) {
                if (const auto aabb = osc::bounding_aabb_of(decorations_, &osc::SceneDecoration::world_space_bounds)) {
                    osc::auto_focus(camera, *aabb, osc::aspect_ratio_of(osc::App::get().main_window_dimensions()));
                }
            }

            // Update the scene camera state based on the user's inputs.
            osc::ui::update_polar_camera_from_all_inputs(
                camera,
                osc::Rect::from_origin_and_dimensions({}, osc::App::get().main_window_dimensions()),
                std::nullopt
            );

            const osc::Vector2 dimensions = osc::App::get().main_window_dimensions();

            osc::SceneRendererParams scene_renderer_params{
                .dimensions = dimensions,
                .device_pixel_ratio = osc::App::get().main_window_device_pixel_ratio(),
                .anti_aliasing_level = osc::App::get().anti_aliasing_level(),
                .draw_floor = draw_floor_,
                .view_matrix = camera.view_matrix(),
                .projection_matrix = camera.projection_matrix(osc::aspect_ratio_of(dimensions)),
                .background_color = background_color_,
            };
            scene_renderer_.render(decorations_, scene_renderer_params);
            ui_context_.render();

            osc::graphics::blit_to_main_window(scene_renderer_.upd_render_texture());
        }

        UiCallbacks callbacks_;
        osc::ui::Context ui_context_{osc::App::upd()};
        std::optional<osc::SceneCache> local_scene_cache_;
        osc::SceneCache* scene_cache_;
        osc::SceneRenderer scene_renderer_{*scene_cache_};
        std::vector<osc::SceneDecoration> decorations_;
        osc::PolarPerspectiveCamera camera;
        osc::Color background_color_ = osc::Color::white();
        bool fit_camera_on_next_frame_ = false;
        bool draw_floor_ = true;
    };
}


void opyn::show_model_in_state(
    OPynSimApp& app,
    const Model& model,
    const ModelState& state,
    osc::Vector2 dimensions,
    osc::Color background_color,
    bool draw_floor,
    osc::SceneCache* scene_cache,
    UiCallbacks callbacks)
{
    app.set_main_window_showing(true);
    app.set_main_window_dimensions(dimensions);
    osc::ScopeExit hide_window_on_exit{[&app]{ app.set_main_window_showing(false); }};
    app.focus_main_window();

    app.show<BasicModelViewer>(
        model,
        state,
        background_color,
        draw_floor,
        scene_cache,
        std::move(callbacks)
    );
}
