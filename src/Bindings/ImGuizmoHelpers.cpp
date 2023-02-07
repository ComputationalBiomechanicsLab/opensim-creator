#include "ImGuizmoHelpers.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Platform/Styling.hpp"
#include "src/Utils/Algorithms.hpp"

#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <ImGuizmo.h>

#include <iterator>

bool osc::DrawGizmoModeSelector(ImGuizmo::MODE& mode)
{
    auto constexpr modeLabels = osc::MakeArray<char const*>("local", "global");
    auto constexpr modes = osc::MakeSizedArray<ImGuizmo::MODE, modeLabels.size()>(ImGuizmo::LOCAL, ImGuizmo::WORLD);

    bool rv = false;
    int currentMode = static_cast<int>(std::distance(std::begin(modes), std::find(std::begin(modes), std::end(modes), mode)));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    ImGui::SetNextItemWidth(ImGui::CalcTextSize(modeLabels[0]).x + 40.0f);
    if (ImGui::Combo("##modeselect", &currentMode, modeLabels.data(), static_cast<int>(modeLabels.size())))
    {
        mode = modes.at(static_cast<size_t>(currentMode));
        rv = true;
    }
    ImGui::PopStyleVar();
    osc::CStringView constexpr tooltipTitle = "Manipulation coordinate system";
    osc::CStringView constexpr tooltipDesc = "This affects whether manipulations (such as the arrow gizmos that you can use to translate things) are performed relative to the global coordinate system or the selection's (local) one. Local manipulations can be handy when translating/rotating something that's already rotated.";
    osc::DrawTooltipIfItemHovered(tooltipTitle, tooltipDesc);

    return rv;
}

bool osc::DrawGizmoOpSelector(
    ImGuizmo::OPERATION& op,
    bool canTranslate,
    bool canRotate,
    bool canScale)
{
    bool rv = false;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0.0f, 0.0f});
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    int colorsPushed = 0;

    if (canTranslate)
    {
        if (op == ImGuizmo::TRANSLATE)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, OSC_NEUTRAL_RGBA);
            ++colorsPushed;
        }
        if (ImGui::Button(ICON_FA_ARROWS_ALT))
        {
            if (op != ImGuizmo::TRANSLATE)
            {
                op = ImGuizmo::TRANSLATE;
                rv = true;
            }
        }
        osc::DrawTooltipIfItemHovered("Translate", "Make the 3D manipulation gizmos translate things (hotkey: G)");
        ImGui::PopStyleColor(std::exchange(colorsPushed, 0));
        ImGui::SameLine();
    }

    if (canRotate)
    {
        if (op == ImGuizmo::ROTATE)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, OSC_NEUTRAL_RGBA);
            ++colorsPushed;
        }
        if (ImGui::Button(ICON_FA_REDO_ALT))
        {
            if (op != ImGuizmo::ROTATE)
            {
                op = ImGuizmo::ROTATE;
                rv = true;
            }
        }
        osc::DrawTooltipIfItemHovered("Rotate", "Make the 3D manipulation gizmos rotate things (hotkey: R)");
        ImGui::PopStyleColor(std::exchange(colorsPushed, 0));
        ImGui::SameLine();
    }

    if (canScale)
    {
        if (op == ImGuizmo::SCALE)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, OSC_NEUTRAL_RGBA);
            ++colorsPushed;
        }
        if (ImGui::Button(ICON_FA_EXPAND_ARROWS_ALT))
        {
            if (op != ImGuizmo::SCALE)
            {
                op = ImGuizmo::SCALE;
                rv = true;
            }
        }
        osc::DrawTooltipIfItemHovered("Scale", "Make the 3D manipulation gizmos scale things (hotkey: S)");
        ImGui::PopStyleColor(std::exchange(colorsPushed, 0));
        ImGui::SameLine();
    }

    ImGui::PopStyleVar(2);

    return rv;
}