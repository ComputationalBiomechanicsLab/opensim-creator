#include "ChecklistPanel.h"

#include <OpenSimCreator/Documents/ModelWarper/ValidationCheck.h>
#include <OpenSimCreator/Documents/ModelWarper/ValidationState.h>
#include <OpenSimCreator/Documents/ModelWarper/WarpableOpenSimComponent.h>
#include <OpenSimCreator/UI/ModelWarper/ModelWarperUIHelpers.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <IconsFontAwesome5.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PhysicalOffsetFrame.h>
#include <oscar/Graphics/Color.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/EnumHelpers.h>

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
    template<WarpableOpenSimComponent T>
    EntryStyling CalcStyle(UIState const& state, T const& c)
    {
        return ToStyle(state.state(c));
    }

    void DrawIcon(EntryStyling const& style)
    {
        ui::PushStyleColor(ImGuiCol_Text, style.color);
        ui::TextUnformatted(style.icon);
        ui::PopStyleColor();
    }

    void DrawEntryIconAndText(
        UIState const&,
        OpenSim::Component const& component,
        EntryStyling style)
    {
        DrawIcon(style);
        ui::SameLine();
        ui::TextUnformatted(component.getName());
    }

    template<WarpableOpenSimComponent T>
    void DrawEntryIconAndText(UIState const& state, T const& c)
    {
        DrawEntryIconAndText(state, c, CalcStyle(state, c));
    }

    void DrawTooltipHeader(UIState const&, OpenSim::Component const& component)
    {
        ui::TextUnformatted(GetAbsolutePathString(component));
        ui::SameLine();
        ui::TextDisabled(component.getConcreteClassName());
        ui::Separator();
        ui::Dummy({0.0f, 3.0f});
    }

    template<WarpableOpenSimComponent T>
    void DrawDetailsTable(UIState const& state, T const& c)
    {
        if (ImGui::BeginTable("##Details", 2)) {

            ImGui::TableSetupColumn("Label");
            ImGui::TableSetupColumn("Value");
            ImGui::TableHeadersRow();

            for (auto&& detail : state.details(c)) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ui::TextUnformatted(detail.name());
                ImGui::TableSetColumnIndex(1);
                ui::TextUnformatted(detail.value());
            }

            ImGui::EndTable();
        }
    }

    template<WarpableOpenSimComponent T>
    void DrawChecklist(UIState const& state, T const& c)
    {
        ImGui::Indent(5.0f);
        int id = 0;
        for (auto&& check : state.validate(c)) {
            ui::PushID(id);
            auto style = ToStyle(check.state());
            DrawIcon(style);
            ui::SameLine();
            ui::TextUnformatted(check.description());
            ui::PopID();
        }
        ImGui::Unindent(5.0f);
    }

    template<WarpableOpenSimComponent T>
    void DrawTooltipContent(UIState const& state, T const& c)
    {
        DrawTooltipHeader(state, c);

        ui::Text("Checklist:");
        ui::Dummy({0.0f, 3.0f});
        DrawChecklist(state, c);

        ImGui::NewLine();

        ui::Text("Details:");
        ui::Dummy({0.0f, 3.0f});
        DrawDetailsTable(state, c);
    }

    template<WarpableOpenSimComponent T>
    void DrawEntry(UIState const& state, T const& c)
    {
        DrawEntryIconAndText(state, c);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) {
            ImGui::BeginTooltip();
            DrawTooltipContent(state, c);
            ImGui::EndTooltip();
        }
    }
}

// UI (meshes/mesh pairing)
namespace
{
    void DrawMeshSectionHeader(UIState const& state)
    {
        ui::Text("Meshes");
        ui::SameLine();
        ui::TextDisabled("(%zu)", GetNumChildren<OpenSim::Mesh>(state.model()));
        ui::SameLine();
        ui::DrawHelpMarker("Shows which meshes are elegible for warping in the source model - and whether the model warper has enough information to warp them (plus any other useful validation checks)");
    }

    void DrawMeshSection(UIState const& state)
    {
        DrawMeshSectionHeader(state);
        ui::Separator();
        int id = 0;
        for (auto const& mesh : state.model().getComponentList<OpenSim::Mesh>()) {
            ui::PushID(id++);
            DrawEntry(state, mesh);
            ui::PopID();
        }
    }
}

// UI (frames)
namespace
{
    void DrawFramesSectionHeader(UIState const& state)
    {
        ui::Text("Warpable Frames");
        ui::SameLine();
        ui::TextDisabled("(%zu)", GetNumChildren<OpenSim::PhysicalOffsetFrame>(state.model()));
        ui::SameLine();
        ui::DrawHelpMarker("Shows which frames are eligible for warping in the source model - and whether the model warper has enough information to warp them");
    }

    void DrawFramesSection(UIState const& state)
    {
        DrawFramesSectionHeader(state);
        ui::Separator();
        int id = 0;
        for (auto const& pof : state.model().getComponentList<OpenSim::PhysicalOffsetFrame>()) {
            ui::PushID(id++);
            DrawEntry(state, pof);
            ui::PopID();
        }
    }
}


// public API

void osc::mow::ChecklistPanel::implDrawContent()
{
    int id = 0;

    ui::PushID(id++);
    DrawMeshSection(*m_State);
    ui::PopID();

    ImGui::NewLine();

    ui::PushID(id++);
    DrawFramesSection(*m_State);
    ui::PopID();
}
