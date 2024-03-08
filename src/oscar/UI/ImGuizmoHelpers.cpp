#include "ImGuizmoHelpers.h"

#include <oscar/Graphics/Color.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/CStringView.h>

#include <IconsFontAwesome5.h>

#include <array>
#include <iterator>
#include <utility>

bool osc::DrawGizmoModeSelector(ImGuizmo::MODE& mode)
{
    constexpr auto modeLabels = std::to_array({ "local", "global" });
    constexpr auto modes = std::to_array<ImGuizmo::MODE, 2>({ ImGuizmo::LOCAL, ImGuizmo::WORLD });

    bool rv = false;
    int currentMode = static_cast<int>(std::distance(std::begin(modes), find(modes, mode)));
    ui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    ui::SetNextItemWidth(ui::CalcTextSize(modeLabels[0]).x + 40.0f);
    if (ui::Combo("##modeselect", &currentMode, modeLabels.data(), static_cast<int>(modeLabels.size())))
    {
        mode = modes.at(static_cast<size_t>(currentMode));
        rv = true;
    }
    ui::PopStyleVar();
    constexpr CStringView tooltipTitle = "Manipulation coordinate system";
    constexpr CStringView tooltipDesc = "This affects whether manipulations (such as the arrow gizmos that you can use to translate things) are performed relative to the global coordinate system or the selection's (local) one. Local manipulations can be handy when translating/rotating something that's already rotated.";
    ui::DrawTooltipIfItemHovered(tooltipTitle, tooltipDesc);

    return rv;
}

bool osc::DrawGizmoOpSelector(
    ImGuizmo::OPERATION& op,
    bool canTranslate,
    bool canRotate,
    bool canScale)
{
    bool rv = false;

    ui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0.0f, 0.0f});
    ui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    int colorsPushed = 0;

    if (canTranslate)
    {
        if (op == ImGuizmo::TRANSLATE)
        {
            ui::PushStyleColor(ImGuiCol_Button, Color::muted_blue());
            ++colorsPushed;
        }
        if (ui::Button(ICON_FA_ARROWS_ALT))
        {
            if (op != ImGuizmo::TRANSLATE)
            {
                op = ImGuizmo::TRANSLATE;
                rv = true;
            }
        }
        ui::DrawTooltipIfItemHovered("Translate", "Make the 3D manipulation gizmos translate things (hotkey: G)");
        ui::PopStyleColor(std::exchange(colorsPushed, 0));
        ui::SameLine();
    }

    if (canRotate)
    {
        if (op == ImGuizmo::ROTATE)
        {
            ui::PushStyleColor(ImGuiCol_Button, Color::muted_blue());
            ++colorsPushed;
        }
        if (ui::Button(ICON_FA_REDO))
        {
            if (op != ImGuizmo::ROTATE)
            {
                op = ImGuizmo::ROTATE;
                rv = true;
            }
        }
        ui::DrawTooltipIfItemHovered("Rotate", "Make the 3D manipulation gizmos rotate things (hotkey: R)");
        ui::PopStyleColor(std::exchange(colorsPushed, 0));
        ui::SameLine();
    }

    if (canScale)
    {
        if (op == ImGuizmo::SCALE)
        {
            ui::PushStyleColor(ImGuiCol_Button, Color::muted_blue());
            ++colorsPushed;
        }
        if (ui::Button(ICON_FA_EXPAND_ARROWS_ALT))
        {
            if (op != ImGuizmo::SCALE)
            {
                op = ImGuizmo::SCALE;
                rv = true;
            }
        }
        ui::DrawTooltipIfItemHovered("Scale", "Make the 3D manipulation gizmos scale things (hotkey: S)");
        ui::PopStyleColor(std::exchange(colorsPushed, 0));
        ui::SameLine();
    }

    ui::PopStyleVar(2);

    return rv;
}

bool osc::UpdateImguizmoStateFromKeyboard(
    ImGuizmo::OPERATION& op,
    ImGuizmo::MODE& mode)
{
    bool const shiftDown = ui::IsShiftDown();
    bool const ctrlOrSuperDown = ui::IsCtrlOrSuperDown();

    if (shiftDown || ctrlOrSuperDown)
    {
        return false;  // assume the user is doing some other action
    }
    else if (ui::IsKeyPressed(ImGuiKey_R))
    {
        // R: set manipulation mode to "rotate"
        if (op == ImGuizmo::ROTATE)
        {
            mode = mode == ImGuizmo::LOCAL ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
        }
        op = ImGuizmo::ROTATE;
        return true;
    }
    else if (ui::IsKeyPressed(ImGuiKey_G))
    {
        // G: set manipulation mode to "grab" (translate)
        if (op == ImGuizmo::TRANSLATE)
        {
            mode = mode == ImGuizmo::LOCAL ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
        }
        op = ImGuizmo::TRANSLATE;
        return true;
}
    else if (ui::IsKeyPressed(ImGuiKey_S))
    {
        // S: set manipulation mode to "scale"
        if (op == ImGuizmo::SCALE)
        {
            mode = mode == ImGuizmo::LOCAL ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
        }
        op = ImGuizmo::SCALE;
        return true;
    }
    else
    {
        return false;
    }
}

void osc::SetImguizmoStyleToOSCStandard()
{
    ImGuizmo::Style& style = ImGuizmo::GetStyle();
    style.TranslationLineThickness = 5.0f;
    style.TranslationLineArrowSize = 8.0f;
    style.RotationLineThickness = 5.0f;
    style.RotationOuterLineThickness = 7.0f;
    style.ScaleLineThickness = 5.0f;
    style.ScaleLineCircleSize = 8.0f;
}
