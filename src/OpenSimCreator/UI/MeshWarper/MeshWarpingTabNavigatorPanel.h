#pragma once

#include <OpenSimCreator/Documents/MeshWarper/TPSDocument.h>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentHelpers.h>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentLandmarkPair.h>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentNonParticipatingLandmark.h>
#include <OpenSimCreator/Platform/OSCColors.h>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabSharedState.h>

#include <oscar/Maths/Circle.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Panels/StandardPanelImpl.h>

#include <memory>
#include <string_view>
#include <utility>

namespace osc
{
    class MeshWarpingTabNavigatorPanel final : public StandardPanelImpl {
    public:
        MeshWarpingTabNavigatorPanel(
            std::string_view label_,
            std::shared_ptr<MeshWarpingTabSharedState> shared_) :

            StandardPanelImpl{label_},
            m_State{std::move(shared_)}
        {
        }
    private:
        void impl_draw_content() final
        {
            ui::draw_text_unformatted("Landmarks:");
            ui::draw_separator();
            if (ContainsLandmarks(m_State->getScratch()))
            {
                drawLandmarksTable();
            }
            else
            {
                ui::draw_text_disabled_and_centered("(none in the scene)");
            }

            ui::start_new_line();

            ui::draw_text_unformatted("Non-Participating Landmarks:");
            ui::draw_separator();
            if (ContainsNonParticipatingLandmarks(m_State->getScratch()))
            {
                drawNonPariticpatingLandmarksTable();
            }
            else
            {
                ui::draw_text_disabled_and_centered("(none in the scene)");
            }
            ui::start_new_line();
        }

        // draws warp-affecting landmarks table. Shows the user:
        //
        // - named landmarks
        // - whether they have source/destination location, or are paired
        void drawLandmarksTable()
        {
            if (!ui::begin_table("##LandmarksTable", 3, getTableFlags()))
            {
                return;
            }

            ui::table_setup_column("Name", {}, 0.7f*ui::get_content_region_available().x);
            ui::table_setup_column("Source", {}, 0.15f*ui::get_content_region_available().x);
            ui::table_setup_column("Destination", {}, 0.15f*ui::get_content_region_available().x);

            int id = 0;
            for (const auto& lm : m_State->getScratch().landmarkPairs)
            {
                ui::push_id(++id);
                drawLandmarksTableRow(lm);
                ui::pop_id();
            }

            ui::end_table();
        }

        void drawLandmarksTableRow(const TPSDocumentLandmarkPair& p)
        {
            // name column
            ui::table_next_row();
            ui::table_set_column_index(0);
            ui::align_text_to_frame_padding();
            ui::draw_text_column_centered(p.name);

            // source column
            ui::table_set_column_index(1);
            const Circle srcCircle = drawLandmarkCircle(
                m_State->isSelected(p.sourceID()),
                m_State->isHovered(p.sourceID()),
                IsFullyPaired(p),
                p.maybeSourceLocation.has_value()
            );

            // destination column
            ui::table_set_column_index(2);
            const Circle destCircle = drawLandmarkCircle(
                m_State->isSelected(p.destinationID()),
                m_State->isHovered(p.destinationID()),
                IsFullyPaired(p),
                p.maybeDestinationLocation.has_value()
            );

            if (IsFullyPaired(p))
            {
                drawConnectingLine(srcCircle, destCircle);
            }
        }

        Circle drawLandmarkCircle(
            bool isSelected,
            bool isHovered,
            bool isPaired,
            bool hasLocation)
        {
            const Circle circle{.origin = calcColumnMidpointScreenPos(), .radius = calcCircleRadius()};
            const Color color = landmarkDotColor(hasLocation, isPaired);

            ui::DrawListView dl = ui::get_panel_draw_list();
            if (hasLocation) {
                dl.add_circle_filled(circle, color);
            }
            else {
                dl.add_circle(circle, color);
            }

            tryDrawCircleHighlight(circle, isSelected, isHovered);

            return circle;
        }

        void tryDrawCircleHighlight(const Circle& circle, bool isSelected, bool isHovered)
        {
            ui::DrawListView dl = ui::get_panel_draw_list();
            const float thickness = 2.0f;
            if (isSelected) {
                dl.add_circle(circle.expanded_by(thickness), OSCColors::selected(), 0, thickness);
            }
            else if (isHovered) {
                dl.add_circle(circle.expanded_by(thickness), OSCColors::hovered(), 0, thickness);
            }
        }

        void drawConnectingLine(const Circle& src, const Circle& dest)
        {
            const float pad = ui::get_style_item_inner_spacing().x;

            // draw connecting line
            const Vec2 direction = normalize(dest.origin - src.origin);
            const Vec2 start = src.origin  + (src.radius  + Vec2{pad, 0.0f})*direction;
            const Vec2 end   = dest.origin - (dest.radius + Vec2{pad, 0.0f})*direction;
            const Color color = Color::half_grey();
            ui::get_panel_draw_list().add_line(start, end, color);

            // draw triangle on end of connecting line to form an arrow
            const Vec2 p0 = end;
            const Vec2 base = p0 - 2.0f*pad*direction;
            const Vec2 orthogonal = {-direction.y, direction.x};
            const Vec2 p1 = base + pad*orthogonal;
            const Vec2 p2 = base - pad*orthogonal;
            ui::get_panel_draw_list().add_triangle_filled(p0, p1, p2, color);
        }

        // draws non-participating landmarks table
        void drawNonPariticpatingLandmarksTable()
        {
            if (!ui::begin_table("##NonParticipatingLandmarksTable", 2, getTableFlags()))
            {
                return;
            }

            ui::table_setup_column("Name", {}, 0.7f*ui::get_content_region_available().x);
            ui::table_setup_column("Location", {}, 0.3f*ui::get_content_region_available().x);

            int id = 0;
            for (const auto& npl : m_State->getScratch().nonParticipatingLandmarks)
            {
                ui::push_id(++id);
                drawNonParticipatingLandmarksTableRow(npl);
                ui::pop_id();
            }

            ui::end_table();
        }

        void drawNonParticipatingLandmarksTableRow(const TPSDocumentNonParticipatingLandmark& npl)
        {
            // name column
            ui::table_next_row();
            ui::table_set_column_index(0);
            ui::align_text_to_frame_padding();
            ui::draw_text_column_centered(npl.name);

            // source column
            ui::table_set_column_index(1);
            drawNonParticipatingLandmarkCircle(
                m_State->isSelected(npl.getID()),
                m_State->isHovered(npl.getID())
            );
        }

        void drawNonParticipatingLandmarkCircle(
            bool isSelected,
            bool isHovered)
        {
            const Circle circle{.origin = calcColumnMidpointScreenPos(), .radius = calcCircleRadius()};

            ui::get_panel_draw_list().add_circle_filled(
                circle,
                m_State->getNonParticipatingLandmarkColor()
            );

            tryDrawCircleHighlight(circle, isSelected, isHovered);
        }

        Color landmarkDotColor(bool hasLocation, bool isPaired) const
        {
            if (hasLocation) {
                if (isPaired) {
                    return m_State->getPairedLandmarkColor();
                }
                else {
                    return m_State->getUnpairedLandmarkColor();
                }
            }
            return Color::half_grey();
        }

        ui::TableFlags getTableFlags() const
        {
            return {ui::TableFlag::NoSavedSettings, ui::TableFlag::SizingStretchSame};
        }

        float calcCircleRadius() const
        {
            return 0.4f*ui::get_text_line_height();
        }

        Vec2 calcColumnMidpointScreenPos() const
        {
            return Vec2{ui::get_cursor_screen_pos()} + Vec2{0.5f*ui::get_column_width(), 0.5f*ui::get_text_line_height()};
        }

        std::shared_ptr<MeshWarpingTabSharedState> m_State;
    };
}
