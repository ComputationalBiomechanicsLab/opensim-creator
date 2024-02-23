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

    template<WarpableOpenSimComponent T>
    void DrawEntryIconAndText(UIState const& state, T const& c)
    {
        DrawEntryIconAndText(state, c, CalcStyle(state, c));
    }

    void DrawTooltipHeader(UIState const&, OpenSim::Component const& component)
    {
        TextUnformatted(GetAbsolutePathString(component));
        ImGui::SameLine();
        ImGui::TextDisabled("%s", component.getConcreteClassName().c_str());
        ImGui::Separator();
        ImGui::Dummy({0.0f, 3.0f});
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
                TextUnformatted(detail.name());
                ImGui::TableSetColumnIndex(1);
                TextUnformatted(detail.value());
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
            ImGui::PushID(id);
            auto style = ToStyle(check.state());
            DrawIcon(style);
            ImGui::SameLine();
            TextUnformatted(check.description());
            ImGui::PopID();
        }
        ImGui::Unindent(5.0f);
    }

    template<WarpableOpenSimComponent T>
    void DrawTooltipContent(UIState const& state, T const& c)
    {
        DrawTooltipHeader(state, c);

        ImGui::Text("Checklist:");
        ImGui::Dummy({0.0f, 3.0f});
        DrawChecklist(state, c);

        ImGui::NewLine();

        ImGui::Text("Details:");
        ImGui::Dummy({0.0f, 3.0f});
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
        ImGui::Text("Meshes");
        ImGui::SameLine();
        ImGui::TextDisabled("(%zu)", GetNumChildren<OpenSim::Mesh>(state.model()));
        ImGui::SameLine();
        DrawHelpMarker("Shows which meshes are elegible for warping in the source model - and whether the model warper has enough information to warp them (plus any other useful validation checks)");
    }

    void DrawMeshSection(UIState const& state)
    {
        DrawMeshSectionHeader(state);
        ImGui::Separator();
        int id = 0;
        for (auto const& mesh : state.model().getComponentList<OpenSim::Mesh>()) {
            ImGui::PushID(id++);
            DrawEntry(state, mesh);
            ImGui::PopID();
        }
    }
}

// UI (frames)
namespace
{
    void DrawFramesSectionHeader(UIState const& state)
    {
        ImGui::Text("Warpable Frames");
        ImGui::SameLine();
        ImGui::TextDisabled("(%zu)", GetNumChildren<OpenSim::PhysicalOffsetFrame>(state.model()));
        ImGui::SameLine();
        DrawHelpMarker("Shows which frames are eligible for warping in the source model - and whether the model warper has enough information to warp them");
    }

    void DrawFramesSection(UIState const& state)
    {
        DrawFramesSectionHeader(state);
        ImGui::Separator();
        int id = 0;
        for (auto const& pof : state.model().getComponentList<OpenSim::PhysicalOffsetFrame>()) {
            ImGui::PushID(id++);
            DrawEntry(state, pof);
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
