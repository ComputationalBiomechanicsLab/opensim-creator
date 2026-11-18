#include "renderer_perf_testing_tab.h"

#include <libopensimcreator/documents/model/undoable_model_actions.h>
#include <libopensimcreator/documents/model/undoable_model_state_pair.h>
#include <libopensimcreator/ui/shared/basic_widgets.h>

#include <libopynsim/graphics/model_renderer_params.h>
#include <libopynsim/graphics/open_sim_decoration_generator.h>
#include <libopynsim/graphics/open_sim_graphics_helpers.h>
#include <liboscar/graphics/graphics.h>
#include <liboscar/graphics/orbit_camera_controller.h>
#include <liboscar/graphics/scene/scene_cache.h>
#include <liboscar/graphics/scene/scene_helpers.h>
#include <liboscar/graphics/scene/scene_renderer.h>
#include <liboscar/graphics/scene/scene_renderer_params.h>
#include <liboscar/maths/aabb_functions.h>
#include <liboscar/maths/angle.h>
#include <liboscar/maths/rect.h>
#include <liboscar/maths/rect_functions.h>
#include <liboscar/maths/vector.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/app_settings.h>
#include <liboscar/platform/log.h>
#include <liboscar/ui/icon_cache.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/tabs/tab_private.h>
#include <liboscar/utilities/c_string_view.h>

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

using namespace osc;
using namespace osc::literals;

namespace
{
    struct FrameTimeAccumulator {
        AppClock::duration mean{0.0};
        size_t count = 0;

        void accumulate(AppClock::duration dur)
        {
            mean = (dur + mean * count) / static_cast<double>(count + 1);
            ++count;
        }

        float fps() const { return 1.0f / static_cast<float>(mean.count()); }

        void reset() { *this = {}; }
    };
}

class osc::RendererPerfTestingTab::Impl final : public TabPrivate {
public:
    Impl(RendererPerfTestingTab& owner, Widget* parent) :
        TabPrivate{owner, parent, "RendererPerfTesting"}
    {
        generate_decorations();
    }

    void on_mount()
    {
        was_v_sync_enabled_ = App::get().vsync_enabled();
        App::upd().set_vsync_enabled(false);
    }

    void on_unmount() const { App::upd().set_vsync_enabled(was_v_sync_enabled_); }

    void on_tick()
    {
        const auto dt = App::get().frame_delta_since_last_frame();
        if (not paused_) {
            orbit_camera_controller_.theta =
                mod(orbit_camera_controller_.theta + (90_deg * dt.count()), 360_deg);
            orbit_camera_controller_.update_camera(model_renderer_params_.camera);
            frame_time_accumulator_.accumulate(dt);
        }
    }

    void on_draw()
    {
        const Rect workspace_screen_rect = ui::get_main_window_workspace_screen_space_rect();

        if (regenerate_decorations_each_frame_) {
            generate_decorations();
        }
        if (std::exchange(first_frame_, false)) {
            if (const std::optional<AABB> scene_aabb =
                    bounding_aabb_of(decorations_, &SceneDecoration::world_space_bounds)) {
                orbit_camera_controller_.focus_on(
                    *scene_aabb,
                    model_renderer_params_.camera,
                    aspect_ratio_of(workspace_screen_rect));
                orbit_camera_controller_.update_camera(model_renderer_params_.camera);
            }
        }

        const SceneRendererParams params = calc_params(workspace_screen_rect);
        renderer_.render(decorations_, params);
        RenderTexture& scene_texture = renderer_.upd_render_texture();
        graphics::blit_to_main_window(scene_texture, workspace_screen_rect);

        ui::begin_panel("stats");
        ui::draw_checkbox("paused", &paused_);
        ui::draw_checkbox("regenerate decorations each frame", &regenerate_decorations_each_frame_);
        ui::draw_text("{}", frame_time_accumulator_.fps());
        ui::same_line();
        if (ui::draw_small_button("reset")) {
            frame_time_accumulator_.reset();
        }
        if (draw_viewer_top_button_row(model_renderer_params_, decorations_, *icon_cache_)) {
            generate_decorations();
        }
        ui::end_panel();
    }

private:
    SceneRendererParams calc_params(const Rect& workspace_screen_rect) const
    {
        return opyn::calc_scene_renderer_params(
            model_renderer_params_,
            workspace_screen_rect.dimensions(),
            App::settings().get_value<float>("graphics/render_scale", 1.0f) *
                App::get().main_window_device_pixel_ratio(),
            App::get().anti_aliasing_level(),
            1.0f);
    }

    void generate_decorations()
    {
        decorations_ = generate_model_decorations(
            scene_cache_, model_, model_renderer_params_.decoration_options);
    }

    bool first_frame_ = true;
    bool was_v_sync_enabled_ = false;
    FrameTimeAccumulator frame_time_accumulator_;
    bool paused_ = false;
    bool regenerate_decorations_each_frame_ = false;

    SceneCache scene_cache_{App::resource_loader()};
    SceneRenderer renderer_{scene_cache_};
    opyn::ModelRendererParams model_renderer_params_;
    OrbitCameraController orbit_camera_controller_;
    std::vector<SceneDecoration> decorations_;

    UndoableModelStatePair model_ = [] {
        UndoableModelStatePair msp;
        const ResourcePath rajagopal_path{
            "OpenSimCreator/models/RajagopalModel/Rajagopal2016.osim"};
        if (const auto rajagopal = App::resource_filepath(rajagopal_path)) {
            msp = UndoableModelStatePair{*rajagopal};
        } else {
            log_error(
                "{}: no such resource found: falling back to a blank model",
                rajagopal_path.string());
        }
        action_enable_all_wrapping_surfaces(msp);
        return msp;
    }();

    std::shared_ptr<IconCache> icon_cache_ = App::singleton<IconCache>(
        App::resource_loader().with_prefix("OpenSimCreator/icons/"),
        ui::get_font_base_size() / 128.0f,
        App::get().highest_device_pixel_ratio());
};

CStringView osc::RendererPerfTestingTab::id()
{
    return "OpenSimCreator/RendererPerfTesting";
}

osc::RendererPerfTestingTab::RendererPerfTestingTab(Widget* parent) :
    Tab{std::make_unique<Impl>(*this, parent)}
{}
void osc::RendererPerfTestingTab::impl_on_mount()
{
    private_data().on_mount();
}
void osc::RendererPerfTestingTab::impl_on_unmount()
{
    private_data().on_unmount();
}
void osc::RendererPerfTestingTab::impl_on_tick()
{
    private_data().on_tick();
}
void osc::RendererPerfTestingTab::impl_on_draw()
{
    private_data().on_draw();
}
