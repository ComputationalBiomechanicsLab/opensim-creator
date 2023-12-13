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

#include <cstddef>
#include <functional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

using osc::BeginTooltip;
using osc::Color;
using osc::CStringView;
using osc::DerivedFrom;
using osc::DrawHelpMarker;
using osc::FindGeometryFileAbsPath;
using osc::GetAbsolutePathString;
using osc::EndTooltip;
using osc::PopStyleColor;
using osc::TextCentered;
using osc::TextUnformatted;
using osc::mow::MeshWarpPairing;
using osc::mow::UIState;

// data stuff
namespace
{
    void ForEachCheck(
        MeshWarpPairing const* maybePairing,
        std::function<MeshWarpPairing::SearchState(MeshWarpPairing::Check)> const& callback)
    {
        if (maybePairing)
        {
            maybePairing->forEachCheck(callback);
        }
        else
        {
            callback({ "no mesh warp pairing found: this is probably an implementation error (maybe reload?)", MeshWarpPairing::State::Error });
        }
    }

    void ForEachDetailIn(
        OpenSim::Mesh const& mesh,
        MeshWarpPairing const* maybePairing,
        std::function<void(MeshWarpPairing::Detail)> const& callback)
    {
        callback({ "OpenSim::Mesh path in the OpenSim::Model", GetAbsolutePathString(mesh) });

        if (maybePairing)
        {
            maybePairing->forEachDetail(callback);
        }
    }

    MeshWarpPairing::State StateOrError(MeshWarpPairing const* maybePairing)
    {
        return maybePairing ? maybePairing->state() : MeshWarpPairing::State::Error;
    }
}

// UI (generic)
namespace
{
    struct EntryStyling final {
        CStringView icon;
        Color color;
    };

    EntryStyling ToStyle(MeshWarpPairing::State s)
    {
        switch (s)
        {
        case MeshWarpPairing::State::Ok:
            return {.icon = ICON_FA_CHECK, .color = Color::green()};
        case MeshWarpPairing::State::Warning:
            return {.icon = ICON_FA_EXCLAMATION, .color = Color::orange()};
        default:
        case MeshWarpPairing::State::Error:
            return {.icon = ICON_FA_TIMES, .color = Color::red()};
        }
    }

    EntryStyling CalcStyle(UIState const& state, OpenSim::Mesh const& mesh)
    {
        return ToStyle(StateOrError(state.findMeshWarp(mesh)));
    }

    EntryStyling CalcStyle(UIState const&, OpenSim::Frame const&)
    {
        return {.icon = ICON_FA_TIMES, .color = Color::red()};
    }

    void DrawIcon(EntryStyling const& style)
    {
        PushStyleColor(ImGuiCol_Text, style.color);
        TextUnformatted(style.icon);
        PopStyleColor();
    }

    void DrawEntryIconAndText(
        UIState const&,
        OpenSim::Component const& component,
        EntryStyling style)
    {
        DrawIcon(style);
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
}

// UI (meshes/mesh pairing)
namespace
{
    void DrawDetailsTable(
        OpenSim::Mesh const& mesh,
        MeshWarpPairing const* maybePairing)
    {
        if (ImGui::BeginTable("##Details", 2))
        {
            ImGui::TableSetupColumn("Label");
            ImGui::TableSetupColumn("Value");
            ImGui::TableHeadersRow();

            ForEachDetailIn(mesh, maybePairing, [](auto detail)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                TextUnformatted(detail.name);
                ImGui::TableSetColumnIndex(1);
                TextUnformatted(detail.value);
            });
            ImGui::EndTable();
        }
    }

    void DrawMeshTooltipChecklist(MeshWarpPairing const* maybePairing)
    {
        ImGui::Indent(5.0f);
        int id = 0;
        ForEachCheck(maybePairing, [&id](auto check)
        {
            ImGui::PushID(id);
            auto style = ToStyle(check.state);
            DrawIcon(style);
            ImGui::SameLine();
            TextUnformatted(check.description);
            ImGui::PopID();
            return MeshWarpPairing::SearchState::Continue;
        });
        ImGui::Unindent(5.0f);
    }

    void DrawTooltipContent(UIState const& state, OpenSim::Mesh const& mesh)
    {
        DrawTooltipHeader(state, mesh);

        MeshWarpPairing const* maybePairing = state.findMeshWarp(mesh);

        ImGui::Text("Checklist:");
        ImGui::Dummy({0.0f, 3.0f});
        DrawMeshTooltipChecklist(maybePairing);

        ImGui::NewLine();

        ImGui::Text("Details:");
        ImGui::Dummy({0.0f, 3.0f});
        DrawDetailsTable(mesh, maybePairing);
    }

    void DrawMeshEntry(UIState const& state, OpenSim::Mesh const& mesh)
    {
        DrawEntryIconAndText(state, mesh);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
        {
            ImGui::BeginTooltip();
            DrawTooltipContent(state, mesh);
            ImGui::EndTooltip();
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
}

// UI (frames)
namespace
{
    void DrawTooltipContent(UIState const& state, OpenSim::Frame const& frame)
    {
        DrawTooltipHeader(state, frame);
    }

    void DrawFrameEntry(UIState const& state, OpenSim::Frame const& frame)
    {
        DrawEntryIconAndText(state, frame);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
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
