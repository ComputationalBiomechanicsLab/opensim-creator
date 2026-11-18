#pragma once

#include <libopensimcreator/documents/mesh_importer/mi_document.h>
#include <libopensimcreator/documents/mesh_importer/mi_mesh.h>
#include <libopensimcreator/ui/mesh_importer/drawable_thing.h>
#include <libopensimcreator/ui/mesh_importer/mesh_importer_hover.h>
#include <libopensimcreator/ui/mesh_importer/mesh_importer_shared_state.h>
#include <libopensimcreator/ui/mesh_importer/mesh_importer_ui_layer.h>

#include <liboscar/maths/vector.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/utilities/c_string_view.h>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

// select 2 mesh points layer
namespace osc
{
    // runtime options for "Select two mesh points" UI layer
    struct Select2MeshPointsOptions final {

        // a function that is called when the implementation detects two points have
        // been clicked
        //
        // the function should return `true` if the points are accepted
        std::function<bool(Vector3, Vector3)> on_two_points_chosen = [](Vector3, Vector3) {
            return true;
        };

        std::string header =
            "choose first (left-click) and second (right click) mesh positions (ESC to cancel)";
    };

    // UI layer that lets the user select two points on a mesh with left-click and
    // right-click
    class Select2MeshPointsLayer final : public MeshImporterUILayer {
    public:
        Select2MeshPointsLayer(
            MeshImporterUILayerHost& parent,
            std::shared_ptr<MeshImporterSharedState> shared,
            Select2MeshPointsOptions options) :

            MeshImporterUILayer{parent},
            shared_{std::move(shared)},
            options_{std::move(options)}
        {}

    private:
        bool is_both_points_selected() const
        {
            return maybe_first_location_ && maybe_second_location_;
        }

        bool is_any_point_selected() const
        {
            return maybe_first_location_ || maybe_second_location_;
        }

        // handle the transition that may occur after the user clicks two points
        void handle_possible_transition_to_next_step()
        {
            if (not is_both_points_selected()) {
                return;  // user hasn't selected two points yet
            }

            bool points_accepted =
                options_.on_two_points_chosen(*maybe_first_location_, *maybe_second_location_);

            if (points_accepted) {
                request_pop();
            } else {
                // points were rejected, so reset them
                maybe_first_location_.reset();
                maybe_second_location_.reset();
            }
        }

        // handle any side-effects of the user interacting with whatever they are
        // hovered over
        void handle_hovertest_side_effects()
        {
            if (not maybe_current_hover_) {
                return;  // nothing hovered
            } else if (ui::is_mouse_clicked(MouseButton::Left)) {
                // LEFT CLICK: set first mouse location
                maybe_first_location_ = maybe_current_hover_.pos;
                handle_possible_transition_to_next_step();
            } else if (ui::is_mouse_clicked(MouseButton::Right)) {
                // RIGHT CLICK: set second mouse location
                maybe_second_location_ = maybe_current_hover_.pos;
                handle_possible_transition_to_next_step();
            }
        }

        // generate 3D drawable geometry for this particular layer
        std::vector<DrawableThing>& generate_drawables()
        {
            drawables_buffer_.clear();

            const MiDocument& mg = shared_->get_model_graph();

            for (const MiMesh& mesh_el : mg.iter<MiMesh>()) {
                drawables_buffer_.emplace_back(shared_->generate_mesh_drawable(mesh_el));
            }

            drawables_buffer_.push_back(shared_->generate_floor_drawable());

            return drawables_buffer_;
        }

        // draw tooltip that pops up when user is moused over a mesh
        void draw_hover_tooltip()
        {
            if (not maybe_current_hover_) {
                return;
            }

            // returns a string representation of a spatial position (e.g. (0.0, 1.0, 3.0))
            const std::string pos = [](const Vector3& position) {
                std::stringstream ss;
                ss.precision(4);
                ss << '(' << position.x() << ", " << position.y() << ", " << position.z() << ')';
                return std::move(ss).str();
            }(maybe_current_hover_.pos);

            ui::begin_tooltip_nowrap();
            ui::draw_text(pos);
            ui::draw_text_disabled(
                "(left-click to assign as first point, right-click to assign as second point)");
            ui::end_tooltip_nowrap();
        }

        // draw 2D overlay over the render, things like connection lines, dots, etc.
        void draw_overlay()
        {
            if (not is_any_point_selected()) {
                return;
            }

            Vector3 clicked_world_pos =
                maybe_first_location_ ? *maybe_first_location_ : *maybe_second_location_;
            Vector2 clicked_scr_pos = shared_->world_pos_to_ui_pos(clicked_world_pos);

            auto color = Color::black();

            ui::DrawListView dl = ui::get_panel_draw_list();
            dl.add_circle_filled({clicked_scr_pos, 5.0f}, color);

            if (!maybe_current_hover_) {
                return;
            }

            Vector2 hover_scr_pos = shared_->world_pos_to_ui_pos(maybe_current_hover_.pos);

            dl.add_circle_filled({hover_scr_pos, 5.0f}, color);
            dl.add_line(clicked_scr_pos, hover_scr_pos, color, 5.0f);
        }

        // draw 2D "choose something" text at the top of the render
        void draw_header_text() const
        {
            if (options_.header.empty()) {
                return;
            }

            const Vector2 padding{10.0f, 10.0f};
            const Vector2 position = shared_->scene_ui_rect().ypd_top_left() + padding;
            ui::get_panel_draw_list().add_text(position, Color::white(), options_.header);
        }

        // draw a user-clickable button for cancelling out of this choosing state
        void draw_cancel_button()
        {
            ui::push_style_var(ui::StyleVar::FramePadding, {10.0f, 10.0f});
            ui::push_style_color(ui::ColorVar::Button, Color::dark_grey());

            const CStringView text = MSMICONS_ARROW_LEFT " Cancel (ESC)";
            const Vector2 margin = {25.0f, 35.0f};
            const Vector2 button_top_left =
                shared_->scene_ui_rect().ypd_bottom_right() - (ui::calc_button_size(text) + margin);

            ui::set_cursor_ui_position(button_top_left);
            if (ui::draw_button(text)) {
                request_pop();
            }

            ui::pop_style_color();
            ui::pop_style_var();
        }

        bool impl_on_event(Event& e) override { return shared_->on_event(e); }

        void impl_tick(float dt) override
        {
            shared_->tick(dt);

            if (ui::is_key_pressed(Key::Escape)) {
                // ESC: user cancelled out
                request_pop();
            }

            bool is_render_hovered = shared_->is_render_hovered();

            if (is_render_hovered) {
                const bool controller_updated = ui::update_orbit_controller_from_mouse_inputs(
                    shared_->upd_camera_controller(), shared_->get_camera(), shared_->scene_dims());
                if (controller_updated) {
                    shared_->get_camera_controller().update_camera(shared_->upd_camera());
                }
            }
        }

        void impl_on_draw() override
        {
            shared_->set_content_region_avail_as_scene_rect();
            std::vector<DrawableThing>& drawables = generate_drawables();
            maybe_current_hover_ = shared_->do_hovertest(drawables);
            handle_hovertest_side_effects();

            shared_->draw_scene(drawables);
            draw_overlay();
            draw_hover_tooltip();
            draw_header_text();
            draw_cancel_button();
        }

        // data that's shared between other UI states
        std::shared_ptr<MeshImporterSharedState> shared_;

        // options for this state
        Select2MeshPointsOptions options_;

        // (maybe) user mouse hover
        MeshImporterHover maybe_current_hover_;

        // (maybe) first mesh location
        std::optional<Vector3> maybe_first_location_;

        // (maybe) second mesh location
        std::optional<Vector3> maybe_second_location_;

        // buffer that's filled with drawable geometry during a drawcall
        std::vector<DrawableThing> drawables_buffer_;
    };
}
