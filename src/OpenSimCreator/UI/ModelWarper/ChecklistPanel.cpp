#include "ChecklistPanel.hpp"

#include <OpenSimCreator/Utils/OpenSimHelpers.hpp>

#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <OpenSim/Simulation/Model/Frame.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Utils/Concepts.hpp>
#include <oscar/Utils/CStringView.hpp>

#include <sstream>
#include <string>
#include <string_view>
#include <utility>

using osc::BeginTooltip;
using osc::Color;
using osc::CStringView;
using osc::DerivedFrom;
using osc::DrawHelpMarker;
using osc::GetAbsolutePathString;
using osc::EndTooltip;
using osc::PopStyleColor;
using osc::TextUnformatted;
using osc::mow::MeshWarpPairing;
using osc::mow::UIState;

namespace
{
    struct EntryStyling final {
        CStringView icon;
        Color color;
    };

    EntryStyling CalcStyle(UIState const& state, OpenSim::Mesh const& mesh)
    {
        MeshWarpPairing const* maybeWarp = state.findMeshWarp(mesh);

        if (!maybeWarp || maybeWarp->getNumFullyPairedLandmarks() == 0)
        {
            return {.icon = ICON_FA_CROSS, .color = Color::red()};
        }
        else if (maybeWarp->hasUnpairedLandmarks())
        {
            return {.icon = ICON_FA_EXCLAMATION, .color = Color::orange()};
        }
        else
        {
            return {.icon = ICON_FA_CHECK, .color = Color::green()};
        }
    }

    EntryStyling CalcStyle(UIState const&, OpenSim::Frame const&)
    {
        return {.icon = ICON_FA_CROSS, .color = Color::red()};
    }

    void DrawEntryIconAndText(
        UIState const&,
        OpenSim::Component const& component,
        EntryStyling style)
    {
        PushStyleColor(ImGuiCol_Text, style.color);
        TextUnformatted(style.icon);
        PopStyleColor();
        ImGui::SameLine();
        TextUnformatted(component.getName());
    }

    template<DerivedFrom<OpenSim::Component> T>
    void DrawEntryIconAndText(UIState const& state, T const& component)
    {
        DrawEntryIconAndText(state, component, CalcStyle(state, component));
    }

    void DrawTooltipHeader(UIState const&, OpenSim::Component const& component)
    {
        TextUnformatted(GetAbsolutePathString(component));
        ImGui::SameLine();
        ImGui::TextDisabled("%s", component.getConcreteClassName().c_str());
        ImGui::Separator();
        ImGui::Dummy({0.0f, 3.0f});
    }

    std::string CalcStatusString(UIState const& state, OpenSim::Mesh const& mesh)
    {
        MeshWarpPairing const* p = state.findMeshWarp(mesh);
        if (!p)
        {
            return "could not find a mesh warp pairing for this mesh: this is probably an implementation error";
        }
        if (!p->hasSourceLandmarksFilepath())
        {
            std::stringstream ss;
            ss << "has no source landmarks file: expected one at: " << p->recommendedSourceLandmarksFilepath();
            return std::move(ss).str();
        }
        if (!p->hasDestinationMeshFilepath())
        {
            std::stringstream ss;
            ss << "has no destination mesh file: one is expected at: " << p->recommendedDestinationMeshFilepath();
            return std::move(ss).str();
        }
        if (!p->hasDestinationLandmarksFilepath())
        {
            std::stringstream ss;
            ss << "has no destination landmarks file: one expected at: " << p->recommendedDestinationLandmarksFilepath();
            return std::move(ss).str();
        }
    }

    void DrawTooltipContent(UIState const& state, OpenSim::Mesh const& mesh)
    {
        DrawTooltipHeader(state, mesh);
    }

    void DrawMeshEntry(UIState const& state, OpenSim::Mesh const& mesh)
    {
        DrawEntryIconAndText(state, mesh);
        if (ImGui::IsItemHovered())
        {
            BeginTooltip();
            DrawTooltipContent(state, mesh);
            EndTooltip();
        }
    }

    void DrawMeshSection(UIState const& state)
    {
        auto ptrs = state.getWarpableMeshes();

        ImGui::Text("Meshes");
        ImGui::SameLine();
        ImGui::TextDisabled("(%zu)", ptrs.size());
        ImGui::SameLine();
        DrawHelpMarker("Shows which meshes are present in the source model, whether they have associated landmarks, and whether there is a known destination mesh");

        ImGui::Separator();

        int id = 0;
        for (OpenSim::Mesh const* mesh : ptrs)
        {
            ImGui::PushID(id++);
            DrawMeshEntry(state, *mesh);
            ImGui::PopID();
        }
    }

    void DrawTooltipContent(UIState const& state, OpenSim::Frame const& frame)
    {
        DrawTooltipHeader(state, frame);
    }

    void DrawFrameEntry(UIState const& state, OpenSim::Frame const& frame)
    {
        DrawEntryIconAndText(state, frame);
        if (ImGui::IsItemHovered())
        {
            BeginTooltip();
            DrawTooltipContent(state, frame);
            EndTooltip();
        }
    }

    void DrawFramesSection(UIState const& state)
    {
        auto ptrs = state.getWarpableFrames();

        ImGui::Text("Frames");
        ImGui::SameLine();
        ImGui::TextDisabled("(%zu)", ptrs.size());
        ImGui::SameLine();
        DrawHelpMarker("Shows frames in the source model, and whether they are warp-able or not");

        ImGui::Separator();

        int id = 0;
        for (OpenSim::Frame const* frame : ptrs)
        {
            ImGui::PushID(id++);
            DrawFrameEntry(state, *frame);
            ImGui::PopID();
        }
    }
}


// public API

void osc::mow::ChecklistPanel::implDrawContent()
{
    int id = 0;

    ImGui::PushID(id++);
    DrawMeshSection(*m_State);
    ImGui::PopID();

    ImGui::NewLine();

    ImGui::PushID(id++);
    DrawFramesSection(*m_State);
    ImGui::PopID();
}
