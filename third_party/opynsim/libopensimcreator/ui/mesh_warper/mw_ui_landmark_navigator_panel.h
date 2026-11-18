#pragma once

#include <libopensimcreator/documents/mesh_warper/mw_document.h>
#include <libopensimcreator/documents/mesh_warper/mw_document_helpers.h>
#include <libopensimcreator/documents/mesh_warper/mw_document_landmark_pair.h>
#include <libopensimcreator/documents/mesh_warper/mw_document_non_participating_landmark.h>
#include <libopensimcreator/platform/open_sim_creator_colors.h>
#include <libopensimcreator/ui/mesh_warper/mw_ui_shared_state.h>

#include <liboscar/maths/circle.h>
#include <liboscar/maths/vector.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/panels/panel.h>

#include <memory>
#include <string_view>
#include <utility>

namespace osc
{
    class MwUiLandmarkNavigatorPanel final : public Panel {
    public:
        MwUiLandmarkNavigatorPanel(
            Widget* parent, std::string_view label, std::shared_ptr<MwUiSharedState> shared) :

            Panel{parent, label},
            state_{std::move(shared)}
        {}

    private:
        void impl_draw_content() override
        {
            ui::draw_text("Landmarks:");
            ui::draw_separator();
            if (contains_landmarks(state_->get_scratch())) {
                draw_landmarks_table();
            } else {
                ui::draw_text_disabled_and_centered("(none in the scene)");
            }

            ui::start_new_line();

            ui::draw_text("Non-Participating Landmarks:");
            ui::draw_separator();
            if (contains_non_participating_landmarks(state_->get_scratch())) {
                draw_non_pariticpating_landmarks_table();
            } else {
                ui::draw_text_disabled_and_centered("(none in the scene)");
            }
            ui::start_new_line();
        }

        // draws warp-affecting landmarks table. Shows the user:
        //
        // - named landmarks
        // - whether they have source/destination location, or are paired
        void draw_landmarks_table()
        {
            if (not ui::begin_table("##LandmarksTable", 3, get_table_flags())) {
                return;
            }

            ui::table_setup_column("Name", {}, 0.70f * ui::get_content_region_available().x());
            ui::table_setup_column("Source", {}, 0.15f * ui::get_content_region_available().x());
            ui::table_setup_column(
                "Destination", {}, 0.15f * ui::get_content_region_available().x());

            int id = 0;
            for (const auto& lm : state_->get_scratch().landmark_pairs) {
                ui::push_id(++id);
                draw_landmarks_table_row(lm);
                ui::pop_id();
            }

            ui::end_table();
        }

        void draw_landmarks_table_row(const MwDocumentLandmarkPair& p)
        {
            // name column
            ui::table_next_row();
            ui::table_set_column_index(0);
            ui::align_text_to_frame_padding();
            ui::draw_text_column_centered(p.name);

            // source column
            ui::table_set_column_index(1);
            const Circle src_circle = draw_landmark_circle(
                state_->is_selected(p.source_id()),
                state_->is_hovered(p.source_id()),
                is_fully_paired(p),
                p.maybe_source_location.has_value());

            // destination column
            ui::table_set_column_index(2);
            const Circle dest_circle = draw_landmark_circle(
                state_->is_selected(p.destination_id()),
                state_->is_hovered(p.destination_id()),
                is_fully_paired(p),
                p.maybe_destination_location.has_value());

            if (is_fully_paired(p)) {
                draw_connecting_line(src_circle, dest_circle);
            }
        }

        Circle draw_landmark_circle(
            bool is_selected, bool is_hovered, bool is_paired, bool has_location)
        {
            const Circle circle{
                .origin = calc_column_midpoint_screen_pos(), .radius = calc_circle_radius()};
            const Color color = landmark_dot_color(has_location, is_paired);

            ui::DrawListView dl = ui::get_panel_draw_list();
            if (has_location) {
                dl.add_circle_filled(circle, color);
            } else {
                dl.add_circle(circle, color);
            }

            try_draw_circle_highlight(circle, is_selected, is_hovered);

            return circle;
        }

        void try_draw_circle_highlight(const Circle& circle, bool is_selected, bool is_hovered)
        {
            ui::DrawListView dl = ui::get_panel_draw_list();
            const float thickness = 2.0f;
            if (is_selected) {
                dl.add_circle(circle.expanded_by(thickness), OSCColors::selected(), 0, thickness);
            } else if (is_hovered) {
                dl.add_circle(circle.expanded_by(thickness), OSCColors::hovered(), 0, thickness);
            }
        }

        void draw_connecting_line(const Circle& src, const Circle& dest)
        {
            const float pad = ui::get_style_item_inner_spacing().x();

            // draw connecting line
            const Vector2 direction = normalize(dest.origin - src.origin);
            const Vector2 start = src.origin + (src.radius + Vector2{pad, 0.0f}) * direction;
            const Vector2 end = dest.origin - (dest.radius + Vector2{pad, 0.0f}) * direction;
            const Color color = Color::dark_grey();
            ui::get_panel_draw_list().add_line(start, end, color);

            // draw triangle on end of connecting line to form an arrow
            const Vector2 p0 = end;
            const Vector2 base = p0 - 2.0f * pad * direction;
            const Vector2 orthogonal = {-direction.y(), direction.x()};
            const Vector2 p1 = base + pad * orthogonal;
            const Vector2 p2 = base - pad * orthogonal;
            ui::get_panel_draw_list().add_triangle_filled(p0, p1, p2, color);
        }

        // draws non-participating landmarks table
        void draw_non_pariticpating_landmarks_table()
        {
            if (not ui::begin_table("##NonParticipatingLandmarksTable", 2, get_table_flags())) {
                return;
            }

            ui::table_setup_column("Name", {}, 0.7f * ui::get_content_region_available().x());
            ui::table_setup_column("Location", {}, 0.3f * ui::get_content_region_available().x());

            int id = 0;
            for (const auto& npl : state_->get_scratch().non_participating_landmarks) {
                ui::push_id(++id);
                draw_non_participating_landmarks_table_row(npl);
                ui::pop_id();
            }

            ui::end_table();
        }

        void draw_non_participating_landmarks_table_row(
            const MwDocumentNonParticipatingLandmark& npl)
        {
            // name column
            ui::table_next_row();
            ui::table_set_column_index(0);
            ui::align_text_to_frame_padding();
            ui::draw_text_column_centered(npl.name);

            // source column
            ui::table_set_column_index(1);
            draw_non_participating_landmark_circle(
                state_->is_selected(npl.get_id()), state_->is_hovered(npl.get_id()));
        }

        void draw_non_participating_landmark_circle(bool is_selected, bool is_hovered)
        {
            const Circle circle{
                .origin = calc_column_midpoint_screen_pos(), .radius = calc_circle_radius()};

            ui::get_panel_draw_list().add_circle_filled(
                circle, state_->get_non_participating_landmark_color());

            try_draw_circle_highlight(circle, is_selected, is_hovered);
        }

        Color landmark_dot_color(bool has_location, bool is_paired) const
        {
            if (has_location) {
                if (is_paired) {
                    return state_->get_paired_landmark_color();
                } else {
                    return state_->get_unpaired_landmark_color();
                }
            }
            return Color::dark_grey();
        }

        ui::TableFlags get_table_flags() const
        {
            return {ui::TableFlag::NoSavedSettings, ui::TableFlag::SizingStretchSame};
        }

        float calc_circle_radius() const
        {
            return 0.4f * ui::get_text_line_height_in_current_panel();
        }

        Vector2 calc_column_midpoint_screen_pos() const
        {
            const float column_width = ui::get_column_width();
            const float line_height = ui::get_text_line_height_in_current_panel();
            return ui::get_cursor_ui_position() + (0.5f * Vector2{column_width, line_height});
        }

        std::shared_ptr<MwUiSharedState> state_;
    };
}
