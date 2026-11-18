#pragma once

#include <libopensimcreator/documents/mesh_warper/mw_undoable_actions.h>
#include <libopensimcreator/platform/msmicons.h>
#include <libopensimcreator/ui/mesh_warper/mw_ui_decoration_generators.h>
#include <libopensimcreator/ui/mesh_warper/mw_ui_panel.h>
#include <libopensimcreator/ui/mesh_warper/mw_ui_shared_state.h>

#include <libopynsim/documents/landmarks/landmark_csv_flags.h>
#include <liboscar/graphics/scene/cached_scene_renderer.h>
#include <liboscar/graphics/scene/scene_cache.h>
#include <liboscar/graphics/scene/scene_decoration.h>
#include <liboscar/graphics/scene/scene_renderer_params.h>
#include <liboscar/graphics/camera.h>
#include <liboscar/graphics/orbit_camera_controller.h>
#include <liboscar/graphics/render_texture.h>
#include <liboscar/maths/vector.h>
#include <liboscar/platform/app.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/utilities/c_string_view.h>

#include <functional>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

namespace osc
{
    // a "result" panel (i.e. after applying a warp to the source)
    class MwUiResultPanel final : public MwUiPanel {
    public:
        explicit MwUiResultPanel(
            Widget* parent, std::string_view panel_name, std::shared_ptr<MwUiSharedState> state) :

            MwUiPanel{parent, panel_name},
            state_{std::move(state)}
        {
            camera_.set_vertical_field_of_view(Degrees{35.0f});
            camera_controller_.focus_on(
                state_->get_result_mesh().bounds().value_or(AABB{}), camera_);
            camera_controller_.update_camera(camera_);
        }

    private:
        void impl_draw_content() override
        {
            // fill the entire available region with the render
            const Vector2 dims = ui::get_content_region_available();

            update_camera();

            // render it via ImGui and hittest it
            RenderTexture& render_texture = render_scene(dims);
            ui::draw_image(render_texture);
            last_texture_hittest_result_ = ui::hittest_last_drawn_item();

            draw_overlays(last_texture_hittest_result_.item_ui_rect);
        }

        void update_camera()
        {
            // if cameras are linked together, ensure all cameras match the "base" camera
            if (state_->update_local_camera_controller_from_linked_base(camera_controller_)) {
                camera_controller_.update_camera(camera_);
            }

            // update camera if user drags it around etc.
            if (last_texture_hittest_result_.is_hovered) {
                if (ui::update_orbit_controller_from_mouse_inputs(
                        camera_controller_,
                        camera_,
                        last_texture_hittest_result_.item_ui_rect.dimensions())) {

                    camera_controller_.update_camera(camera_);
                    state_->set_linked_camera_controller(
                        camera_controller_);  // reflects latest modification
                }
            }
        }

        // draw ImGui overlays over a result panel
        void draw_overlays(const Rect& render_rect)
        {
            // ImGui: set cursor to draw over the top-right of the render texture (with padding)
            ui::set_cursor_ui_position(render_rect.ypd_top_left() + overlay_padding_);

            draw_information_icon();
            ui::same_line();
            draw_export_button();
            ui::same_line();
            draw_auto_fit_camera_button();
            ui::same_line();
            draw_landmark_radius_slider();
            draw_blending_factor_slider();
            draw_prescale_inputs();

            // align with "export" button in row above
            ui::set_cursor_panel_x(cursor_x_at_export_button_);
            ui::draw_checkbox("overlay destination mesh", &show_destination_mesh_);
            ui::same_line();
            {
                bool recalculating_normals = state_->get_scratch().recalculate_normals;
                if (ui::draw_checkbox("recalculate mesh's normals", &recalculating_normals)) {
                    action_set_recalculating_normals(state_->upd_undoable(), recalculating_normals);
                }
            }
        }

        // draws a information icon that shows basic mesh info when hovered
        void draw_information_icon()
        {
            ui::draw_button_nobg(MSMICONS_INFO_CIRCLE);
            if (ui::is_item_hovered()) {
                ui::begin_tooltip();

                ui::draw_text_disabled("Result Information:");
                draw_information_table();

                ui::end_tooltip();
            }
        }

        // draws a table containing useful input information (handy for debugging)
        void draw_information_table()
        {
            if (ui::begin_table("##inputinfo", 2)) {
                ui::table_setup_column("Name");
                ui::table_setup_column("Value");

                ui::table_next_row();
                ui::table_set_column_index(0);
                ui::draw_text("# vertices");
                ui::table_set_column_index(1);
                ui::draw_text("{}", state_->get_result_mesh().num_vertices());

                ui::table_next_row();
                ui::table_set_column_index(0);
                ui::draw_text("# triangles");
                ui::table_set_column_index(1);
                ui::draw_text("{}", state_->get_result_mesh().num_indices() / 3);

                ui::end_table();
            }
        }

        // draws an export button that enables the user to export things from this input
        void draw_export_button()
        {
            // needed to align the blending factor slider
            cursor_x_at_export_button_ = ui::get_cursor_panel_x();
            ui::draw_button(MSMICONS_FILE_EXPORT " export" MSMICONS_CARET_DOWN);
            if (ui::begin_popup_context_menu(
                    "##exportcontextmenu", ui::PopupFlag::MouseButtonLeft)) {

                if (ui::draw_menu_item("Mesh to OBJ")) {
                    action_prompt_user_to_save_mesh_to_obj_file(
                        state_->get_result_mesh(), OBJWriterFlag::Default);
                }
                if (ui::draw_menu_item("Mesh to OBJ (no normals)")) {
                    action_prompt_user_to_save_mesh_to_obj_file(
                        state_->get_result_mesh(), OBJWriterFlag::NoWriteNormals);
                }
                if (ui::draw_menu_item("Mesh to STL")) {
                    action_prompt_user_to_mesh_to_stl_file(state_->get_result_mesh());
                }
                if (ui::draw_menu_item("Warped Non-Participating Landmarks to CSV")) {
                    action_prompt_user_to_save_warped_non_participating_landmarks_to_csv(
                        state_->get_scratch(), state_->upd_result_cache());
                }
                if (ui::draw_menu_item("Warped Non-Participating Landmark Positions to CSV")) {
                    action_prompt_user_to_save_warped_non_participating_landmarks_to_csv(
                        state_->get_scratch(),
                        state_->upd_result_cache(),
                        opyn::LandmarkCSVFlags::NoHeader | opyn::LandmarkCSVFlags::NoNames);
                }
                if (ui::draw_menu_item("Landmark Pairs to CSV")) {
                    action_prompt_user_to_save_paired_landmarks_to_csv(state_->get_scratch());
                }
                if (ui::draw_menu_item("Landmark Pairs to CSV (no names)")) {
                    action_prompt_user_to_save_paired_landmarks_to_csv(
                        state_->get_scratch(), opyn::LandmarkCSVFlags::NoNames);
                }
                ui::end_popup();
            }
        }

        // draws a button that auto-fits the camera to the 3D scene
        void draw_auto_fit_camera_button()
        {
            if (ui::draw_button(MSMICONS_EXPAND_ARROWS_ALT)) {
                camera_controller_.focus_on(
                    state_->get_result_mesh().bounds().value_or(AABB{}),
                    camera_,
                    aspect_ratio_of(last_texture_hittest_result_.item_ui_rect));
                camera_controller_.update_camera(camera_);
                state_->set_linked_camera_controller(camera_controller_);
            }
            ui::draw_tooltip_if_item_hovered(
                "Autoscale Scene",
                "Zooms camera to try and fit everything in the scene into the viewer");
        }

        // draws a slider that lets the user edit how large the landmarks are
        void draw_landmark_radius_slider()
        {
            // note: log scale is important: some users have meshes that
            // are in different scales (e.g. millimeters)
            const ui::SliderFlags flags = ui::SliderFlag::Logarithmic;

            const CStringView label = "landmark radius";
            ui::set_next_item_width(
                ui::get_content_region_available().x() - ui::calc_text_size(label).x() -
                ui::get_style_item_inner_spacing().x() - state_->get_overlay_padding().x());
            ui::draw_float_slider(label, &landmark_radius_, 0.0001f, 100.0f, "%.4f", flags);
        }

        void draw_blending_factor_slider()
        {
            // align with "export" button in row above
            ui::set_cursor_panel_x(cursor_x_at_export_button_);

            // deliberate trailing spaces (for alignment with "landmark radius").
            const CStringView label = "blending factor  ";
            ui::set_next_item_width(
                ui::get_content_region_available().x() - ui::calc_text_size(label).x() -
                ui::get_style_item_inner_spacing().x() - overlay_padding_.x());

            float factor = state_->get_scratch().blending_factor;
            if (ui::draw_float_slider(label, &factor, 0.0f, 1.0f)) {
                action_set_blend_factor_without_committing(state_->upd_undoable(), factor);
            }
            if (ui::is_item_deactivated_after_edit()) {
                action_set_blend_factor(state_->upd_undoable(), factor);
            }
        }

        void draw_prescale_inputs()
        {
            float warping_penalty = state_->get_scratch().warping_penalty;
            ui::set_cursor_panel_x(cursor_x_at_export_button_);
            ui::draw_float_input("warping penalty", &warping_penalty);
            if (ui::is_item_deactivated_after_edit()) {
                action_set_warping_penalty(state_->upd_undoable(), warping_penalty);
            }

            float source_prescale = state_->get_scratch().source_landmarks_prescale;
            ui::set_cursor_panel_x(cursor_x_at_export_button_);
            ui::draw_float_input("source landmarks prescale", &source_prescale);
            if (ui::is_item_deactivated_after_edit()) {
                action_set_source_landmarks_prescale(state_->upd_undoable(), source_prescale);
            }
            float destination_prescale = state_->get_scratch().destination_landmarks_prescale;
            ui::set_cursor_panel_x(cursor_x_at_export_button_);
            ui::draw_float_input("destination prescale", &destination_prescale);
            if (ui::is_item_deactivated_after_edit()) {
                action_set_destination_landmarks_prescale(
                    state_->upd_undoable(), destination_prescale);
            }

            ui::set_cursor_panel_x(cursor_x_at_export_button_);
            bool affine_scale = state_->get_scratch().apply_affine_scale;
            ui::draw_checkbox("scale", &affine_scale);
            if (ui::is_item_deactivated_after_edit()) {
                // TODO: undo/redo
                state_->upd_undoable().upd_scratch().apply_affine_scale = affine_scale;
            }
            ui::same_line();
            bool affine_rotation = state_->get_scratch().apply_affine_rotation;
            ui::draw_checkbox("rotate", &affine_rotation);
            if (ui::is_item_deactivated_after_edit()) {
                // TODO: undo/redo
                state_->upd_undoable().upd_scratch().apply_affine_rotation = affine_rotation;
            }
            ui::same_line();
            bool affine_translation = state_->get_scratch().apply_affine_translation;
            ui::draw_checkbox("translate", &affine_translation);
            if (ui::is_item_deactivated_after_edit()) {
                // TODO: undo/redo
                state_->upd_undoable().upd_scratch().apply_affine_translation = affine_translation;
            }
            ui::same_line();
            bool non_affine_warp = state_->get_scratch().apply_non_affine_warp;
            ui::draw_checkbox("warp", &non_affine_warp);
            if (ui::is_item_deactivated_after_edit()) {
                // TODO: undo/redo
                state_->upd_undoable().upd_scratch().apply_non_affine_warp = non_affine_warp;
            }
        }

        // returns 3D decorations for the given result panel
        std::vector<SceneDecoration> generate_decorations() const
        {
            std::vector<SceneDecoration> decorations;
            const std::function<void(SceneDecoration&&)> decoration_consumer =
                [&decorations](SceneDecoration&& dec) {
                    decorations.push_back(std::move(dec));
                };

            append_common_decorations(
                *state_,
                state_->get_result_mesh(),
                state_->is_wireframe_mode_enabled(),
                decoration_consumer);

            if (show_destination_mesh_) {
                decoration_consumer(
                    SceneDecoration{
                        .mesh = state_->get_scratch().destination_mesh,
                        .shading = Color::red().with_alpha(0.5f),
                    });
            }

            // draw non-participating landmarks
            for (const Vector3& non_participating_landmark_pos :
                 state_->get_result_non_participating_landmark_locations()) {

                const auto sf = get_non_participating_landmark_scale_factor();
                decoration_consumer(
                    SceneDecoration{
                        .mesh = state_->get_landmark_sphere_mesh(),
                        .transform =
                            {
                                .scale = Vector3{sf * landmark_radius_},
                                .translation = non_participating_landmark_pos,
                            },
                        .shading = state_->get_non_participating_landmark_color(),
                    });
            }

            return decorations;
        }

        // renders a panel to a texture via its renderer and returns a reference to the rendered
        // texture
        RenderTexture& render_scene(Vector2 dims)
        {
            const std::vector<SceneDecoration> decorations = generate_decorations();
            SceneRendererParams params = calc_standard_dark_scene_render_params(
                camera_,
                App::get().anti_aliasing_level(),
                dims,
                App::settings().get_value<float>("graphics/render_scale", 1.0f) *
                    App::get().main_window_device_pixel_ratio());
            state_->get_custom_rendering_options().apply_to(params);
            return cached_renderer_.render(decorations, params);
        }

        std::shared_ptr<MwUiSharedState> state_;
        Camera camera_;
        OrbitCameraController camera_controller_;
        CachedSceneRenderer cached_renderer_{
            *App::singleton<SceneCache>(App::resource_loader()),
        };
        ui::HittestResult last_texture_hittest_result_;
        bool show_destination_mesh_ = false;
        Vector2 overlay_padding_ = {10.0f, 10.0f};
        float landmark_radius_ = 0.05f;
        float cursor_x_at_export_button_ = 0.0f;
    };
}
