#pragma once

#include <OpenSimCreator/Documents/MeshWarper/TPSDocument.hpp>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentHelpers.hpp>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentLandmarkPair.hpp>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentNonParticipatingLandmark.hpp>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabSharedState.hpp>

#include <imgui.h>
#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Maths/Circle.hpp>
#include <oscar/Maths/Vec2.hpp>
#include <oscar/UI/Panels/StandardPanel.hpp>

#include <memory>
#include <string_view>
#include <utility>

namespace osc
{
    class MeshWarpingTabNavigatorPanel final : public StandardPanel {
    public:
        MeshWarpingTabNavigatorPanel(
            std::string_view label_,
            std::shared_ptr<MeshWarpingTabSharedState> shared_) :
            StandardPanel{label_},
            m_Shared{std::move(shared_)}
        {
        }
    private:
        void implDrawContent() final
        {
            ImGui::TextUnformatted("Landmarks:");
            ImGui::Separator();
            if (ContainsLandmarks(m_Shared->getScratch()))
            {
                drawLandmarksTable();
            }
            else
            {
                TextDisabledAndCentered("(none in the scene)");
            }

            ImGui::NewLine();

            ImGui::TextUnformatted("Non-Participating Landmarks:");
            ImGui::Separator();
            if (ContainsNonParticipatingLandmarks(m_Shared->getScratch()))
            {
                drawNonPariticpatingLandmarksTable();
            }
            else
            {
                TextDisabledAndCentered("(none in the scene)");
            }
            ImGui::NewLine();
        }

        // draws warp-affecting landmarks table. Shows the user:
        //
        // - named landmarks
        // - whether they have source/destination location, or are paired
        void drawLandmarksTable()
        {
            if (!ImGui::BeginTable("##LandmarksTable", 3, getTableFlags()))
            {
                return;
            }

            ImGui::TableSetupColumn("Name", 0, 0.7f*ImGui::GetContentRegionAvail().x);
            ImGui::TableSetupColumn("Source", 0, 0.15f*ImGui::GetContentRegionAvail().x);
            ImGui::TableSetupColumn("Destination", 0, 0.15f*ImGui::GetContentRegionAvail().x);
            ImGui::TableHeadersRow();

            int id = 0;
            for (auto const& lm : m_Shared->getScratch().landmarkPairs)
            {
                ImGui::PushID(++id);
                drawLandmarksTableRow(lm);
                ImGui::PopID();
            }

            ImGui::EndTable();
        }

        void drawLandmarksTableRow(TPSDocumentLandmarkPair const& p)
        {
            // name column
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            TextColumnCentered(p.id.c_str());

            bool const fullyPaired = IsFullyPaired(p);

            // source column
            ImGui::TableSetColumnIndex(1);
            Circle const srcCircle = tryDrawLandmarkDot(fullyPaired, p.maybeSourceLocation);

            // destination column
            ImGui::TableSetColumnIndex(2);
            Circle const destCircle = tryDrawLandmarkDot(fullyPaired, p.maybeDestinationLocation);

            if (fullyPaired)
            {
                drawConnectingLine(srcCircle, destCircle);
            }
        }

        Circle tryDrawLandmarkDot(
            bool paired,
            std::optional<Vec3> const& maybeLocation)
        {
            Color const textColor =
                maybeLocation ?
                    (paired ? m_Shared->pairedLandmarkColor : m_Shared->unpairedLandmarkColor) :
                    Color::halfGrey();

            Vec2 const midpoint = calcColumnMidpointScreenPos();
            ImU32 const textImGuiColor = ToImU32(textColor);
            float const radius = calcCircleRadius();

            auto& dl = *ImGui::GetWindowDrawList();
            if (maybeLocation)
            {
                dl.AddCircleFilled(midpoint, radius, textImGuiColor);
            }
            else
            {
                dl.AddCircle(midpoint, radius, textImGuiColor);
            }

            return Circle{.origin = midpoint, .radius = radius};
        }

        void drawConnectingLine(Circle const& src, Circle const& dest)
        {
            float const pad = ImGui::GetStyle().ItemInnerSpacing.x;

            // draw connecting line
            Vec2 const direction = Normalize(dest.origin - src.origin);
            Vec2 const start = src.origin  + (src.radius  + Vec2{pad, 0.0f})*direction;
            Vec2 const end   = dest.origin - (dest.radius + Vec2{pad, 0.0f})*direction;
            ImU32 const color = ToImU32(Color::halfGrey());
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
            if (!ImGui::BeginTable("##NonParticipatingLandmarksTable", 2, getTableFlags()))
            {
                return;
            }

            ImGui::TableSetupColumn("Name", 0, 0.7f*ImGui::GetContentRegionAvail().x);
            ImGui::TableSetupColumn("Location", 0, 0.3f*ImGui::GetContentRegionAvail().x);
            ImGui::TableHeadersRow();

            int id = 0;
            for (auto const& npl : m_Shared->getScratch().nonParticipatingLandmarks)
            {
                ImGui::PushID(++id);
                drawNonParticipatingLandmarksTableRow(npl);
                ImGui::PopID();
            }

            ImGui::EndTable();
        }

        void drawNonParticipatingLandmarksTableRow(TPSDocumentNonParticipatingLandmark const& npl)
        {
            // name column
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            TextColumnCentered(npl.id.c_str());

            // source column
            ImGui::TableSetColumnIndex(1);
            ImGui::GetWindowDrawList()->AddCircleFilled(
                calcColumnMidpointScreenPos(),
                calcCircleRadius(),
                ToImU32(m_Shared->nonParticipatingLandmarkColor)
            );
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

        std::shared_ptr<MeshWarpingTabSharedState> m_Shared;
    };
}
