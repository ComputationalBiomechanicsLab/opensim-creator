#include "splash_tab.h"

#include <libopensimcreator/documents/model/undoable_model_actions.h>
#include <libopensimcreator/platform/msmicons.h>
#include <libopensimcreator/platform/recent_file.h>
#include <libopensimcreator/platform/recent_files.h>
#include <libopensimcreator/ui/loading_tab.h>
#include <libopensimcreator/ui/mesh_importer/mesh_importer_tab.h>
#include <libopensimcreator/ui/mesh_warper/mw_ui_tab.h>
#include <libopensimcreator/ui/model_warper/model_warper_tab.h>
#include <libopensimcreator/ui/preview_experimental_data/preview_experimental_data_tab.h>
#include <libopensimcreator/ui/shared/main_menu.h>

#include <libopynsim/utilities/open_sim_helpers.h>
#include <liboscar/formats/svg.h>
#include <liboscar/graphics/scene/scene_cache.h>
#include <liboscar/graphics/scene/scene_renderer.h>
#include <liboscar/graphics/scene/scene_renderer_params.h>
#include <liboscar/graphics/camera.h>
#include <liboscar/graphics/color.h>
#include <liboscar/graphics/orbit_camera_controller.h>
#include <liboscar/graphics/texture2d.h>
#include <liboscar/graphics/texture_filter_mode.h>
#include <liboscar/maths/math_helpers.h>
#include <liboscar/maths/rect.h>
#include <liboscar/maths/rect_functions.h>
#include <liboscar/maths/vector.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/app_metadata.h>
#include <liboscar/platform/app_settings.h>
#include <liboscar/platform/events/drop_file_event.h>
#include <liboscar/platform/events/event.h>
#include <liboscar/platform/os.h>
#include <liboscar/ui/events/open_tab_event.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/tabs/tab_private.h>
#include <liboscar/utilities/algorithms.h>
#include <liboscar/utilities/c_string_view.h>

#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <utility>

using namespace osc::literals;
using namespace osc;

namespace
{
    Camera get_splash_screen_default_camera()
    {
        Camera rv;
        rv.set_vertical_field_of_view(35_deg);
        OrbitCameraController controller{.radius = 10.0f, .theta = 45_deg, .phi = 30_deg};
        controller.update_camera(rv);
        return rv;
    }

    SceneRendererParams get_splash_screen_default_render_params(const Camera& camera)
    {
        const auto clipping_planes = camera.clipping_planes();
        SceneRendererParams rv;
        rv.draw_rims = false;
        rv.view_matrix = camera.view_matrix();
        rv.near_clipping_plane = clipping_planes.znear;
        rv.far_clipping_plane = clipping_planes.zfar;
        rv.viewer_position = camera.position();
        rv.light_direction = {-0.34f, -0.25f, 0.05f};
        rv.light_color = {248.0f / 255.0f, 247.0f / 255.0f, 247.0f / 255.0f, 1.0f};
        rv.background_color = {0.89f, 0.89f, 0.89f, 1.0f};
        return rv;
    }

    // helper: draws an ui::draw_menu_item for a given recent- or example-file-path
    void draw_recent_or_example_file_menu_item(
        const std::filesystem::path& path, Widget& parent, int& imgui_id)
    {
        const std::string label = std::string{MSMICONS_FILE} + " " + path.filename().string();

        ui::push_id(++imgui_id);
        if (ui::draw_menu_item(label)) {
            auto tab = std::make_unique<LoadingTab>(&parent, path);
            App::post_event<OpenTabEvent>(parent, std::move(tab));
        }
        // show the full path as a tooltip when the item is hovered (some people have
        // long file names (#784)
        if (ui::is_item_hovered()) {
            ui::begin_tooltip_nowrap();
            ui::draw_text(path.filename().string());
            ui::end_tooltip_nowrap();
        }
        ui::pop_id();
    }
}

class osc::SplashTab::Impl final : public TabPrivate {
public:
    explicit Impl(SplashTab& owner, Widget* parent) :
        TabPrivate{owner, parent, MSMICONS_HOME},
        main_menu_file_tab_{std::make_unique<MainMenuFileTab>(&owner)}
    {
        main_app_logo_.set_filter_mode(TextureFilterMode::Linear);
        czi_logo_.set_filter_mode(TextureFilterMode::Linear);
        tud_logo_.set_filter_mode(TextureFilterMode::Linear);
    }

    void on_mount()
    {
        // edge-case: reset the file tab whenever the splash screen is (re)mounted,
        // because actions within other tabs may have updated things like recently
        // used files etc. (#618)
        main_menu_file_tab_ = std::make_unique<MainMenuFileTab>(&owner());

        App::upd().make_main_loop_waiting();
    }

    void on_unmount() { App::upd().make_main_loop_polling(); }

    bool on_event(Event& e)
    {
        if (const auto* dropfile = dynamic_cast<const DropFileEvent*>(&e)) {
            if (opyn::has_model_file_extension(dropfile->path())) {
                auto tab = std::make_unique<LoadingTab>(&owner(), dropfile->path());
                App::post_event<OpenTabEvent>(owner(), std::move(tab));
                return true;
            }
        }
        return false;
    }

    void draw_main_menu()
    {
        main_menu_file_tab_->on_draw();
        main_menu_about_tab_.on_draw();
    }

    void on_draw()
    {
        if (not ui::main_window_has_workspace()) {
            // edge-case: splash screen is the first rendered frame and ImGui
            //            is being unusual about it
            return;
        }

        draw_background();
        draw_logo();
        draw_attributation_logos();
        draw_version_info();
        draw_menu();
    }

private:
    Rect calc_main_menu_rect() const
    {
        const auto tab_ui_rect_corners = ui::get_main_window_workspace_ui_rect().corners();
        const Rect tab_ui_rect_without_bar = Rect::from_corners(
            tab_ui_rect_corners.min,
            tab_ui_rect_corners.max -
                Vector2{
                    0.0f,
                    max(tud_logo_.dimensions().y(), czi_logo_.dimensions().y()) -
                        2.0f * ui::get_style_panel_padding().y()});

        const Vector2 menu_and_top_logo_dims = elementwise_min(
            tab_ui_rect_without_bar.dimensions(),
            Vector2{
                splash_menu_max_dims_.x(),
                splash_menu_max_dims_.y() + main_app_logo_dims_.y() + top_logo_padding_.y()});
        const Vector2 menu_and_top_logo_top_left =
            tab_ui_rect_without_bar.ypd_top_left() +
            0.5f * (tab_ui_rect_without_bar.dimensions() - menu_and_top_logo_dims);
        const Vector2 menu_dims = {
            menu_and_top_logo_dims.x(),
            menu_and_top_logo_dims.y() - main_app_logo_dims_.y() - top_logo_padding_.y()};
        const Vector2 menu_top_left = Vector2{
            menu_and_top_logo_top_left.x(),
            menu_and_top_logo_top_left.y() + main_app_logo_dims_.y() + top_logo_padding_.y()};

        return Rect::from_corners(menu_top_left, menu_top_left + menu_dims);
    }

    Rect calc_logo_rect() const
    {
        const Rect mmr = calc_main_menu_rect();
        const Vector2 top_left{
            mmr.left() + 0.5f * mmr.width() - 0.5f * main_app_logo_dims_.x(),
            mmr.ypd_top() - top_logo_padding_.y() - main_app_logo_dims_.y(),
        };

        return Rect::from_corners(top_left, top_left + main_app_logo_dims_);
    }

    void draw_background()
    {
        const Rect workspace_ui_rect = ui::get_main_window_workspace_ui_rect();

        ui::set_next_panel_ui_position(workspace_ui_rect.ypd_top_left());
        ui::set_next_panel_size(workspace_ui_rect.dimensions());

        ui::push_style_var(ui::StyleVar::PanelPadding, {0.0f, 0.0f});
        ui::begin_panel("##splashscreenbackground", nullptr, ui::get_minimal_panel_flags());
        ui::pop_style_var();

        SceneRendererParams params{last_scene_renderer_params_};
        params.dimensions = workspace_ui_rect.dimensions();
        params.device_pixel_ratio =
            App::settings().get_value<float>("graphics/render_scale", 1.0f) *
            App::get().main_window_device_pixel_ratio(),
        params.anti_aliasing_level = App::get().anti_aliasing_level();
        params.projection_matrix = camera_.projection_matrix(aspect_ratio_of(workspace_ui_rect));

        if (params != last_scene_renderer_params_) {
            scene_renderer_.render({}, params);
            last_scene_renderer_params_ = params;
        }

        ui::draw_image(scene_renderer_.upd_render_texture());

        ui::end_panel();
    }

    void draw_logo()
    {
        const Rect logo_rect = calc_logo_rect();

        ui::set_next_panel_ui_position(logo_rect.ypd_top_left());
        ui::begin_panel("##osclogo", nullptr, ui::get_minimal_panel_flags());
        ui::draw_image(main_app_logo_, logo_rect.dimensions());
        ui::end_panel();
    }

    void draw_menu()
    {
        // center the menu window
        const Rect mmr = calc_main_menu_rect();
        const Vector2 dims = mmr.dimensions();
        ui::set_next_panel_ui_position(mmr.ypd_top_left());
        ui::set_next_panel_size({dims.x(), -1.0f});
        ui::set_next_panel_size_constraints(dims, dims);

        if (ui::begin_panel("Splash screen", nullptr, ui::PanelFlag::NoTitleBar)) {
            draw_menu_content();
        }
        ui::end_panel();
    }

    void draw_menu_content()
    {
        // de-dupe imgui IDs because these lists may contain duplicate
        // names
        int imgui_id = 0;

        ui::set_num_columns(2, std::nullopt, false);
        draw_menu_left_column_content(imgui_id);
        ui::next_column();
        draw_menu_right_column_content(imgui_id);
        ui::next_column();
        ui::set_num_columns();
    }

    void draw_actions_menu_section_content()
    {
        if (ui::draw_menu_item(MSMICONS_FILE " New Model")) {
            action_new_model(*parent());
        }
        if (ui::draw_menu_item(MSMICONS_FOLDER_OPEN " Open Model")) {
            action_open_model(*parent());
        }
        if (ui::draw_menu_item(MSMICONS_FILE_IMPORT " Import Meshes")) {
            auto tab = std::make_unique<MeshImporterTab>(parent());
            App::post_event<OpenTabEvent>(*parent(), std::move(tab));
        }
        ui::add_screenshot_annotation_to_last_drawn_item("SplashTab/ImportMeshesMenuItem");
        if (const auto docs_url = App::get().metadata().documentation_url()) {
            if (ui::draw_menu_item(MSMICONS_BOOK " Open Documentation")) {
                open_url_in_os_default_web_browser(*docs_url);
            }
        }
        constexpr CStringView joss_url = "https://joss.theoj.org/papers/10.21105/joss.08284";
        if (ui::draw_menu_item(MSMICONS_LINK " Cite our JOSS paper")) {
            open_url_in_os_default_web_browser(joss_url);
            set_clipboard_text(joss_url);
        }
        ui::draw_tooltip_if_item_hovered(
            joss_url,
            "Click to open in your default browser and, additionally, copy the link to the clipboard (in case the browser doesn't open)");
    }

    void draw_workflows_menu_section_content()
    {
        if (ui::draw_menu_item(MSMICONS_FILE_IMPORT " Mesh Importer")) {
            auto tab = std::make_unique<MeshImporterTab>(parent());
            App::post_event<OpenTabEvent>(*parent(), std::move(tab));
        }

        if (ui::draw_menu_item(MSMICONS_BEZIER_CURVE " Preview Experimental Data")) {
            auto tab = std::make_unique<PreviewExperimentalDataTab>(parent());
            App::post_event<OpenTabEvent>(*parent(), std::move(tab));
        }
        if (ui::draw_menu_item(MSMICONS_CUBE " Mesh Warping")) {
            auto tab = std::make_unique<MwUiTab>(parent());
            App::post_event<OpenTabEvent>(*parent(), std::move(tab));
        }
        ui::add_screenshot_annotation_to_last_drawn_item("SplashTab/MeshWarpingMenuItem");

        if (ui::draw_menu_item(MSMICONS_MAGIC " Model Warping (" MSMICONS_MAGIC " experimental)")) {
            auto tab = std::make_unique<ModelWarperTab>(parent());
            App::post_event<OpenTabEvent>(*parent(), std::move(tab));
        }
        ui::add_screenshot_annotation_to_last_drawn_item("SplashTab/ModelWarpingMenuItem");
    }

    void draw_recently_opened_files_menu_section_content(int& imgui_id)
    {
        const auto recent_files = App::singleton<RecentFiles>();
        if (not recent_files->empty()) {
            for (const RecentFile& rf : *recent_files) {
                draw_recent_or_example_file_menu_item(rf.path, *parent(), imgui_id);
            }
        } else {
            ui::push_style_color(ui::ColorVar::Text, Color::dark_grey());
            ui::draw_text_wrapped("No files opened recently. Try:");
            ui::draw_text_bullet_pointed("Creating a new model (Ctrl+N)");
            ui::draw_text_bullet_pointed("Opening an existing model (Ctrl+O)");
            ui::draw_text_bullet_pointed("Opening an example (right-side)");
            ui::pop_style_color();
        }
    }

    void draw_menu_left_column_content(int& imgui_id)
    {
        ui::draw_text_disabled("Actions");
        ui::draw_vertical_spacer(2.0f / 15.0f);

        draw_actions_menu_section_content();

        ui::draw_vertical_spacer(1.0f);
        ui::draw_text_disabled("Workflows");
        ui::draw_vertical_spacer(2.0f / 15.0f);

        draw_workflows_menu_section_content();

        ui::draw_vertical_spacer(1.0f);
        ui::draw_text_disabled("Recent Models");
        ui::draw_vertical_spacer(2.0f / 15.0f);

        draw_recently_opened_files_menu_section_content(imgui_id);
    }

    void draw_menu_right_column_content(int& imgui_id)
    {
        if (not main_menu_file_tab_->example_osim_files.empty()) {

            ui::draw_text_disabled("Example Models");
            ui::draw_vertical_spacer(2.0f / 15.0f);

            for (const std::filesystem::path& example_path :
                 main_menu_file_tab_->example_osim_files) {

                draw_recent_or_example_file_menu_item(example_path, *parent(), imgui_id);
            }
        }
    }

    void draw_attributation_logos()
    {
        const Rect workspace_ui_rect = ui::get_main_window_workspace_ui_rect();
        Vector2 loc = workspace_ui_rect.ypd_bottom_right();
        loc.x() = loc.x() - 2.0f * ui::get_style_panel_padding().x() - czi_logo_.dimensions().x() -
                  2.0f * ui::get_style_item_spacing().x() - tud_logo_.dimensions().x();
        loc.y() = loc.y() - 2.0f * ui::get_style_panel_padding().y() -
                  max(czi_logo_.dimensions().y(), tud_logo_.dimensions().y());

        ui::set_next_panel_ui_position(loc);
        ui::begin_panel("##czlogo", nullptr, ui::get_minimal_panel_flags());
        ui::draw_image(czi_logo_);
        ui::end_panel();

        loc.x() += czi_logo_.dimensions().x() + 2.0f * ui::get_style_item_spacing().x();
        ui::set_next_panel_ui_position(loc);
        ui::begin_panel("##tudlogo", nullptr, ui::get_minimal_panel_flags());
        ui::draw_image(tud_logo_);
        ui::end_panel();
    }

    void draw_version_info()
    {
        const Rect tab_ui_rect = ui::get_main_window_workspace_ui_rect();
        const float h = ui::get_font_base_size_with_spacing();
        const float padding = 5.0f;

        const Vector2 position{
            tab_ui_rect.left() + padding, tab_ui_rect.ypd_bottom() - h - padding};

        ui::DrawListView dl = ui::get_foreground_draw_list();
        const std::string text = App::get().application_name_with_version_and_buildid();
        dl.add_text(position, Color::black(), text);
    }

    // for rendering the 3D scene
    Camera camera_ = get_splash_screen_default_camera();
    SceneRenderer scene_renderer_{
        *App::singleton<SceneCache>(App::resource_loader()),
    };
    SceneRendererParams last_scene_renderer_params_ =
        get_splash_screen_default_render_params(camera_);

    Texture2D main_app_logo_ = SVG::read_into_texture(
        App::open_resource("OpenSimCreator/textures/banner.svg"),
        1.0f,
        App::get().highest_device_pixel_ratio());
    Texture2D czi_logo_ = SVG::read_into_texture(
        App::open_resource("OpenSimCreator/textures/chanzuckerberg_logo.svg"),
        0.5f,
        App::get().highest_device_pixel_ratio());
    Texture2D tud_logo_ = SVG::read_into_texture(
        App::open_resource("OpenSimCreator/textures/tudelft_logo.svg"),
        0.5f,
        App::get().highest_device_pixel_ratio());

    // dimensions of stuff
    Vector2 splash_menu_max_dims_ = {640.0f, 512.0f};
    Vector2 main_app_logo_dims_ = main_app_logo_.dimensions();
    Vector2 top_logo_padding_ = {25.0f, 35.0f};

    // UI state
    std::unique_ptr<MainMenuFileTab> main_menu_file_tab_;
    MainMenuAboutTab main_menu_about_tab_;
};

osc::SplashTab::SplashTab(Widget* parent) :
    Tab{std::make_unique<Impl>(*this, parent)}
{}
void osc::SplashTab::impl_on_mount()
{
    private_data().on_mount();
}
void osc::SplashTab::impl_on_unmount()
{
    private_data().on_unmount();
}
bool osc::SplashTab::impl_on_event(Event& e)
{
    return private_data().on_event(e);
}
void osc::SplashTab::impl_on_draw_main_menu()
{
    private_data().draw_main_menu();
}
void osc::SplashTab::impl_on_draw()
{
    private_data().on_draw();
}
