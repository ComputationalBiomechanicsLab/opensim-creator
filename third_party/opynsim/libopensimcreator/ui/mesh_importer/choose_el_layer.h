#pragma once

#include <libopensimcreator/documents/mesh_importer/mi_document.h>
#include <libopensimcreator/documents/mesh_importer/mi_ids.h>
#include <libopensimcreator/documents/mesh_importer/mi_object.h>
#include <libopensimcreator/ui/mesh_importer/drawable_thing.h>
#include <libopensimcreator/ui/mesh_importer/mesh_importer_hover.h>
#include <libopensimcreator/ui/mesh_importer/mesh_importer_shared_state.h>
#include <libopensimcreator/ui/mesh_importer/mesh_importer_ui_layer.h>

#include <liboscar/graphics/color.h>
#include <liboscar/graphics/scene/scene_decoration_flags.h>
#include <liboscar/maths/easing_functions.h>
#include <liboscar/maths/vector.h>
#include <liboscar/platform/app.h>
#include <liboscar/shims/cpp23/ranges.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/uid.h>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

// choose specific element layer
namespace osc
{
    // options for when the UI transitions into "choose something" mode
    struct ChooseElLayerOptions final {

        // types of elements the user can choose in this screen
        bool can_choose_bodies = true;
        bool can_choose_ground = true;
        bool can_choose_meshes = true;
        bool can_choose_joints = true;
        bool can_choose_stations = false;

        // (maybe) elements the assignment is ultimately assigning
        std::unordered_set<UID> maybe_els_attaching_to;

        // false means the user is attaching "away from" their selection (used for drawing arrows)
        bool is_attaching_toward_el = true;

        // (maybe) elements that are being replaced by the user's choice
        std::unordered_set<UID> maybe_els_being_replaced_by_choice;

        // the number of elements the user must click before on_user_choice is called
        int num_elements_user_must_choose = 1;

        // function that returns true if the "caller" is happy with the user's choice
        std::function<bool(std::span<UID>)> on_user_choice = [](std::span<UID>) {
            return true;
        };

        // user-facing header text
        std::string header = "choose something";
    };

    // "choose `n` things" UI layer
    //
    // this is what's drawn when the user's being prompted to choose scene elements
    class ChooseElLayer final : public MeshImporterUILayer {
    public:
        ChooseElLayer(
            MeshImporterUILayerHost& parent,
            std::shared_ptr<MeshImporterSharedState> shared,
            ChooseElLayerOptions options) :

            MeshImporterUILayer{parent},
            shared_{std::move(shared)},
            options_{std::move(options)}
        {}

    private:
        // returns true if the user's mouse is hovering over the given scene element
        bool is_hovered(const MiObject& el) const { return el.get_id() == maybe_hover_.id; }

        // returns true if the user has already selected the given scene element
        bool is_selected(const MiObject& el) const
        {
            return cpp23::contains(selected_object_ids_, el.get_id());
        }

        // returns true if the user can (de)select the given element
        bool is_selectable(const MiObject& el) const
        {
            if (options_.maybe_els_attaching_to.contains(el.get_id())) {
                return false;
            }

            return std::visit(
                Overload{
                    [this](const MiGround&) { return options_.can_choose_ground; },
                    [this](const MiMesh&) { return options_.can_choose_meshes; },
                    [this](const MiBody&) { return options_.can_choose_bodies; },
                    [this](const MiJoint&) { return options_.can_choose_joints; },
                    [this](const MiStation&) { return options_.can_choose_stations; },
                },
                el.to_variant());
        }

        void select(const MiObject& el)
        {
            if (!is_selectable(el)) {
                return;
            }

            if (is_selected(el)) {
                return;
            }

            selected_object_ids_.push_back(el.get_id());
        }

        void de_select(const MiObject& el)
        {
            if (!is_selectable(el)) {
                return;
            }

            std::erase_if(
                selected_object_ids_, [el_id = el.get_id()](UID id) { return id == el_id; });
        }

        void try_toggle_selection_state_of(const MiObject& el)
        {
            is_selected(el) ? de_select(el) : select(el);
        }

        void try_toggle_selection_state_of(UID id)
        {
            const MiObject* el = shared_->get_model_graph().try_get_by_id(id);

            if (el) {
                try_toggle_selection_state_of(*el);
            }
        }

        SceneDecorationFlags compute_flags(const MiObject& el) const
        {
            SceneDecorationFlags rv = SceneDecorationFlag::None;
            if (is_selected(el)) {
                rv |= SceneDecorationFlag::RimHighlight0;
            }
            if (is_hovered(el)) {
                rv |= SceneDecorationFlag::RimHighlight1;
            }
            return rv;
        }

        // returns a list of 3D drawable scene objects for this layer
        std::vector<DrawableThing>& generate_drawables()
        {
            drawables_buffer_.clear();

            const MiDocument& mg = shared_->get_model_graph();

            float faded_alpha = 0.2f;
            float anim_scale = ease_out_elastic(animation_fraction_);

            for (const MiObject& el : mg.iter()) {
                size_t start = drawables_buffer_.size();
                shared_->append_drawables(el, drawables_buffer_);
                size_t end = drawables_buffer_.size();

                bool is_selectable_el = is_selectable(el);
                SceneDecorationFlags flags = compute_flags(el);

                for (size_t i = start; i < end; ++i) {
                    DrawableThing& d = drawables_buffer_[i];
                    d.flags = flags;

                    if (not is_selectable_el) {
                        if (std::holds_alternative<Color>(d.shading)) {
                            // fade non-selectable colored scene elements
                            std::get<Color>(d.shading).a = faded_alpha;
                        }
                        d.id = MiIDs::empty();
                        d.group_id = MiIDs::empty();
                    } else {
                        d.transform.scale *= anim_scale;
                    }
                }
            }

            // floor
            drawables_buffer_.push_back(shared_->generate_floor_drawable());

            return drawables_buffer_;
        }

        void handle_possible_completion()
        {
            if (static_cast<int>(selected_object_ids_.size()) <
                options_.num_elements_user_must_choose) {

                return;  // user hasn't selected enough stuff yet
            }

            if (options_.on_user_choice(selected_object_ids_)) {
                request_pop();
            } else {
                // choice was rejected?
            }
        }

        // handle any side-effects from the user's mouse hover
        void handle_hovertest_side_effects()
        {
            if (!maybe_hover_) {
                return;
            }

            draw_hover_tooltip();

            if (ui::is_mouse_clicked(MouseButton::Left)) {
                try_toggle_selection_state_of(maybe_hover_.id);
                handle_possible_completion();
            }
        }

        // draw 2D tooltip that pops up when user is hovered over something in the scene
        void draw_hover_tooltip() const
        {
            if (!maybe_hover_) {
                return;
            }

            const MiObject* se = shared_->get_model_graph().try_get_by_id(maybe_hover_.id);

            if (se) {
                ui::begin_tooltip_nowrap();
                ui::draw_text(se->get_label());
                ui::same_line();
                ui::draw_text_disabled("({}, click to choose)", se->get_class().get_name());
                ui::end_tooltip_nowrap();
            }
        }

        // draw 2D connection overlay lines that show what's connected to what in the graph
        //
        // depends on layer options
        void draw_connection_lines() const
        {
            if (!maybe_hover_) {
                // user isn't hovering anything, so just draw all existing connection
                // lines, but faintly
                shared_->draw_connection_lines(
                    faintify_color(shared_->get_color_connection_line()));
                return;
            }

            // else: user is hovering *something*

            // draw all other connection lines but exclude the thing being assigned (if any)
            shared_->draw_connection_lines(
                faintify_color(shared_->get_color_connection_line()),
                options_.maybe_els_being_replaced_by_choice);

            // draw strong connection line between the things being attached to and the hover
            for (UID el_attaching_to : options_.maybe_els_attaching_to) {
                Vector3 parent_pos = shared_->get_model_graph().get_pos_by_id(el_attaching_to);
                Vector3 child_pos = shared_->get_model_graph().get_pos_by_id(maybe_hover_.id);

                if (!options_.is_attaching_toward_el) {
                    std::swap(parent_pos, child_pos);
                }

                shared_->draw_connection_line(
                    shared_->get_color_connection_line(), parent_pos, child_pos);
            }
        }

        // draw 2D header text in top-left corner of the screen
        void draw_header_text() const
        {
            if (options_.header.empty()) {
                return;
            }

            const Vector2 padding = Vector2{10.0f, 10.0f};
            const Vector2 pos = shared_->scene_ui_rect().ypd_top_left() + padding;
            ui::get_panel_draw_list().add_text(pos, Color::white(), options_.header);
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

            if (animation_fraction_ < 1.0f) {
                animation_fraction_ = saturate(animation_fraction_ + 0.5f * dt);
                App::upd().request_redraw();
            }
        }

        void impl_on_draw() override
        {
            shared_->set_content_region_avail_as_scene_rect();

            std::vector<DrawableThing>& drawables = generate_drawables();

            maybe_hover_ = shared_->do_hovertest(drawables);
            handle_hovertest_side_effects();

            shared_->draw_scene(drawables);
            draw_connection_lines();
            draw_header_text();
            draw_cancel_button();
        }

        Color faintify_color(const Color& src_color) const
        {
            Color color = src_color;
            color.a *= 0.2f;
            return color;
        }

        // data that's shared between other UI states
        std::shared_ptr<MeshImporterSharedState> shared_;

        // options for this state
        ChooseElLayerOptions options_;

        // (maybe) user mouse hover
        MeshImporterHover maybe_hover_;

        // elements selected by user
        std::vector<UID> selected_object_ids_;

        // buffer that's filled with drawable geometry during a drawcall
        std::vector<DrawableThing> drawables_buffer_;

        // fraction that the system is through its animation cycle: ranges from 0.0 to 1.0 inclusive
        float animation_fraction_ = 0.0f;
    };
}
