#include "ChecklistPanel.h"

#include <OpenSimCreator/Documents/ModelWarper/ValidationCheck.h>
#include <OpenSimCreator/Documents/ModelWarper/ValidationCheckConsumerResponse.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/PhysicalOffsetFrame.h>
#include <oscar/Graphics/Color.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/Utils/CStringView.h>

#include <concepts>
#include <cstddef>
#include <functional>
#include <string>
#include <string_view>

using namespace osc;
using namespace osc::mow;

// UI (generic)
namespace
{
    struct EntryStyling final {
        CStringView icon;
        Color color;
    };

    EntryStyling ToStyle(ValidationCheck::State s)
    {
        switch (s) {
        case ValidationCheck::State::Ok:
            return {.icon = ICON_FA_CHECK, .color = Color::green()};
        case ValidationCheck::State::Warning:
            return {.icon = ICON_FA_EXCLAMATION, .color = Color::orange()};
        default:
        case ValidationCheck::State::Error:
            return {.icon = ICON_FA_TIMES, .color = Color::red()};
        }
    }

    EntryStyling CalcStyle(UIState const& state, OpenSim::Mesh const& mesh)
    {
        return ToStyle(state.warpState(mesh));
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

    template<std::derived_from<OpenSim::Component> T>
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
    void DrawDetailsTable(UIState const& state, OpenSim::Mesh const& mesh)
    {
        if (ImGui::BeginTable("##Details", 2)) {

            ImGui::TableSetupColumn("Label");
            ImGui::TableSetupColumn("Value");
            ImGui::TableHeadersRow();

            state.forEachWarpDetail(mesh, [](auto detail)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                TextUnformatted(detail.name());
                ImGui::TableSetColumnIndex(1);
                TextUnformatted(detail.value());
            });
            ImGui::EndTable();
        }
    }

    void DrawMeshTooltipChecklist(UIState const& state, OpenSim::Mesh const& mesh)
    {
        ImGui::Indent(5.0f);
        int id = 0;
        state.forEachWarpCheck(mesh, [&id](auto check)
        {
            ImGui::PushID(id);
            auto style = ToStyle(check.state());
            DrawIcon(style);
            ImGui::SameLine();
            TextUnformatted(check.description());
            ImGui::PopID();
            return ValidationCheckConsumerResponse::Continue;
        });
        ImGui::Unindent(5.0f);
    }

    void DrawTooltipContent(UIState const& state, OpenSim::Mesh const& mesh)
    {
        DrawTooltipHeader(state, mesh);

        ImGui::Text("Checklist:");
        ImGui::Dummy({0.0f, 3.0f});
        DrawMeshTooltipChecklist(state, mesh);

        ImGui::NewLine();

        ImGui::Text("Details:");
        ImGui::Dummy({0.0f, 3.0f});
        DrawDetailsTable(state, mesh);
    }

    void DrawMeshEntry(UIState const& state, OpenSim::Mesh const& mesh)
    {
        DrawEntryIconAndText(state, mesh);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) {
            ImGui::BeginTooltip();
            DrawTooltipContent(state, mesh);
            ImGui::EndTooltip();
        }
    }

    void DrawMeshSection(UIState const& state)
    {
        ImGui::Text("Meshes");
        ImGui::SameLine();
        ImGui::TextDisabled("(%zu)", GetNumChildren<OpenSim::Mesh>(state.model()));
        ImGui::SameLine();
        DrawHelpMarker("Shows which meshes are elegible for warping in the source model - and whether the model warper has enough information to warp them (plus any other useful validation checks)");

        ImGui::Separator();

        int id = 0;
        for (auto const& mesh : state.model().getComponentList<OpenSim::Mesh>()) {
            ImGui::PushID(id++);
            DrawMeshEntry(state, mesh);
            ImGui::PopID();
        }
    }
}

// UI (frames)
namespace
{
    void DrawTooltipChecklist(UIState const& state, OpenSim::PhysicalOffsetFrame const& frame)
    {
        ImGui::Indent(5.0f);
        int id = 0;
        state.forEachWarpCheck(frame, [&id](auto check)
        {
            ImGui::PushID(id);
            auto style = ToStyle(check.state());
            DrawIcon(style);
            ImGui::SameLine();
            TextUnformatted(check.description());
            ImGui::PopID();
            return ValidationCheckConsumerResponse::Continue;
        });
        ImGui::Unindent(5.0f);
    }

    void DrawTooltipContent(
        UIState const& state,
        OpenSim::PhysicalOffsetFrame const& frame)
    {
        DrawTooltipHeader(state, frame);

        ImGui::Text("Checklist:");
        ImGui::Dummy({0.0f, 3.0f});
        DrawTooltipChecklist(state, frame);
    }

    void DrawChecklistEntry(
        UIState const& state,
        OpenSim::PhysicalOffsetFrame const& frame)
    {
        DrawEntryIconAndText(state, frame);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) {
            BeginTooltip();
            DrawTooltipContent(state, frame);
            EndTooltip();
        }
    }

    void DrawFramesSectionHeader(size_t numWarpableFrames)
    {
        ImGui::Text("Warpable Frames");
        ImGui::SameLine();
        ImGui::TextDisabled("(%zu)", numWarpableFrames);
        ImGui::SameLine();
        DrawHelpMarker("Shows which frames are eligible for warping in the source model - and whether the model warper has enough information to warp them");
    }

    void DrawFramesSection(UIState const& state)
    {
        DrawFramesSectionHeader(GetNumChildren<OpenSim::PhysicalOffsetFrame>(state.model()));

        ImGui::Separator();

        int id = 0;
        for (auto const& pof : state.model().getComponentList<OpenSim::PhysicalOffsetFrame>()) {
            ImGui::PushID(id++);
            DrawChecklistEntry(state, pof);
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
