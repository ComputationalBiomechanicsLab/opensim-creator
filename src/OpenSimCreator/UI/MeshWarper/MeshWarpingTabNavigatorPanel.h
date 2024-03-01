#pragma once

#include <OpenSimCreator/Documents/MeshWarper/TPSDocument.h>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentHelpers.h>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentLandmarkPair.h>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentNonParticipatingLandmark.h>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabSharedState.h>

#include <oscar/Maths/Circle.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/UI/ImGuiHelpers.h>
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
        void implDrawContent() final
        {
            ui::TextUnformatted("Landmarks:");
            ui::Separator();
            if (ContainsLandmarks(m_State->getScratch()))
            {
                drawLandmarksTable();
            }
            else
            {
                ui::TextDisabledAndCentered("(none in the scene)");
            }

            ImGui::NewLine();

            ui::TextUnformatted("Non-Participating Landmarks:");
            ui::Separator();
            if (ContainsNonParticipatingLandmarks(m_State->getScratch()))
            {
                drawNonPariticpatingLandmarksTable();
            }
            else
            {
                ui::TextDisabledAndCentered("(none in the scene)");
            }
            ImGui::NewLine();
        }

        // draws warp-affecting landmarks table. Shows the user:
        //
        // - named landmarks
        // - whether they have source/destination location, or are paired
        void drawLandmarksTable()
        {
            if (!ui::BeginTable("##LandmarksTable", 3, getTableFlags()))
            {
                return;
            }

            ui::TableSetupColumn("Name", 0, 0.7f*ImGui::GetContentRegionAvail().x);
            ui::TableSetupColumn("Source", 0, 0.15f*ImGui::GetContentRegionAvail().x);
            ui::TableSetupColumn("Destination", 0, 0.15f*ImGui::GetContentRegionAvail().x);

            int id = 0;
            for (auto const& lm : m_State->getScratch().landmarkPairs)
            {
                ui::PushID(++id);
                drawLandmarksTableRow(lm);
                ui::PopID();
            }

            ui::EndTable();
        }

        void drawLandmarksTableRow(TPSDocumentLandmarkPair const& p)
        {
            // name column
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ui::TextColumnCentered(p.name);

            // source column
            ImGui::TableSetColumnIndex(1);
            Circle const srcCircle = drawLandmarkCircle(
                m_State->isSelected(p.sourceID()),
                m_State->isHovered(p.sourceID()),
                IsFullyPaired(p),
                p.maybeSourceLocation.has_value()
            );

            // destination column
            ImGui::TableSetColumnIndex(2);
            Circle const destCircle = drawLandmarkCircle(
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
            Circle const circle{.origin = calcColumnMidpointScreenPos(), .radius = calcCircleRadius()};
            ImU32 const color = ui::ToImU32(landmarkDotColor(hasLocation, isPaired));

            auto& dl = *ImGui::GetWindowDrawList();
            if (hasLocation)
            {
                dl.AddCircleFilled(circle.origin, circle.radius, color);
            }
            else
            {
                dl.AddCircle(circle.origin, circle.radius, color);
            }

            tryDrawCircleHighlight(circle, isSelected, isHovered);

            return circle;
        }

        void tryDrawCircleHighlight(Circle const& circle, bool isSelected, bool isHovered)
        {
            auto& dl = *ImGui::GetWindowDrawList();
            float const thickness = 2.0f;
            if (isSelected)
            {
                dl.AddCircle(circle.origin, circle.radius + thickness, ui::ToImU32(Color::yellow()), 0, thickness);
            }
            else if (isHovered)
            {
                dl.AddCircle(circle.origin, circle.radius + thickness, ui::ToImU32(Color::yellow().withAlpha(0.5f)), 0, thickness);
            }
        }

        void drawConnectingLine(Circle const& src, Circle const& dest)
        {
            float const pad = ImGui::GetStyle().ItemInnerSpacing.x;

            // draw connecting line
            Vec2 const direction = normalize(dest.origin - src.origin);
            Vec2 const start = src.origin  + (src.radius  + Vec2{pad, 0.0f})*direction;
            Vec2 const end   = dest.origin - (dest.radius + Vec2{pad, 0.0f})*direction;
            ImU32 const color = ui::ToImU32(Color::halfGrey());
            ImGui::GetWindowDrawList()->AddLine(start, end, color);

            // draw triangle on end of connecting line to form an arrow
            Vec2 const p0 = end;
            Vec2 const base = p0 - 2.0f*pad*direction;
            Vec2 const orthogonal = {-direction.y, direction.x};
            Vec2 const p1 = base + pad*orthogonal;
            Vec2 const p2 = base - pad*orthogonal;
            ImGui::GetWindowDrawList()->AddTriangleFilled(p0, p1, p2, color);
        }

        // draws non-participating landmarks table
        void drawNonPariticpatingLandmarksTable()
        {
            if (!ui::BeginTable("##NonParticipatingLandmarksTable", 2, getTableFlags()))
            {
                return;
            }

            ui::TableSetupColumn("Name", 0, 0.7f*ImGui::GetContentRegionAvail().x);
            ui::TableSetupColumn("Location", 0, 0.3f*ImGui::GetContentRegionAvail().x);

            int id = 0;
            for (auto const& npl : m_State->getScratch().nonParticipatingLandmarks)
            {
                ui::PushID(++id);
                drawNonParticipatingLandmarksTableRow(npl);
                ui::PopID();
            }

            ui::EndTable();
        }

        void drawNonParticipatingLandmarksTableRow(TPSDocumentNonParticipatingLandmark const& npl)
        {
            // name column
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ui::TextColumnCentered(npl.name);

            // source column
            ImGui::TableSetColumnIndex(1);
            drawNonParticipatingLandmarkCircle(
                m_State->isSelected(npl.getID()),
                m_State->isHovered(npl.getID())
            );
        }

        void drawNonParticipatingLandmarkCircle(
            bool isSelected,
            bool isHovered)
        {
            Circle const circle{.origin = calcColumnMidpointScreenPos(), .radius = calcCircleRadius()};

            ImGui::GetWindowDrawList()->AddCircleFilled(
                circle.origin,
                circle.radius,
                ui::ToImU32(m_State->nonParticipatingLandmarkColor)
            );

            tryDrawCircleHighlight(circle, isSelected, isHovered);
        }

        Color landmarkDotColor(bool hasLocation, bool isPaired) const
        {
            if (hasLocation)
            {
                if (isPaired)
                {
                    return m_State->pairedLandmarkColor;
                }
                else
                {
                    return m_State->unpairedLandmarkColor;
                }
            }
            return Color::halfGrey();
        }

        ImGuiTableFlags getTableFlags() const
        {
            return
                ImGuiTableFlags_NoSavedSettings |
                ImGuiTableFlags_SizingStretchSame;
        }

        float calcCircleRadius() const
        {
            return 0.4f*ImGui::GetTextLineHeight();
        }

        Vec2 calcColumnMidpointScreenPos() const
        {
            return Vec2{ImGui::GetCursorScreenPos()} + Vec2{0.5f*ImGui::GetColumnWidth(), 0.5f*ImGui::GetTextLineHeight()};
        }

        std::shared_ptr<MeshWarpingTabSharedState> m_State;
    };
}
