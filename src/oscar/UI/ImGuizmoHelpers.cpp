#include "ImGuizmoHelpers.h"

#include <oscar/Graphics/Color.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/CStringView.h>

#include <IconsFontAwesome5.h>

#include <array>
#include <iterator>
#include <ranges>
#include <utility>

namespace rgs = std::ranges;

bool osc::DrawGizmoModeSelector(ImGuizmo::MODE& mode)
{
    constexpr auto mode_labels = std::to_array({ "local", "global" });
    constexpr auto modes = std::to_array<ImGuizmo::MODE, 2>({ ImGuizmo::LOCAL, ImGuizmo::WORLD });

    bool rv = false;
    int current_mode = static_cast<int>(std::distance(rgs::begin(modes), rgs::find(modes, mode)));
    ui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    ui::SetNextItemWidth(ui::CalcTextSize(mode_labels[0]).x + 40.0f);
    if (ui::Combo("##modeselect", &current_mode, mode_labels.data(), static_cast<int>(mode_labels.size()))) {
        mode = modes.at(static_cast<size_t>(current_mode));
        rv = true;
    }
    ui::PopStyleVar();
    constexpr CStringView tooltip_title = "Manipulation coordinate system";
    constexpr CStringView tooltip_description = "This affects whether manipulations (such as the arrow gizmos that you can use to translate things) are performed relative to the global coordinate system or the selection's (local) one. Local manipulations can be handy when translating/rotating something that's already rotated.";
    ui::DrawTooltipIfItemHovered(tooltip_title, tooltip_description);

    return rv;
}

bool osc::DrawGizmoOpSelector(
    ImGuizmo::OPERATION& op,
    bool can_translate,
    bool can_rotate,
    bool can_scale)
{
    bool rv = false;

    ui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0.0f, 0.0f});
    ui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    int num_colors_pushed = 0;

    if (can_translate) {
        if (op == ImGuizmo::TRANSLATE) {
            ui::PushStyleColor(ImGuiCol_Button, Color::muted_blue());
            ++num_colors_pushed;
        }
        if (ui::Button(ICON_FA_ARROWS_ALT)) {
            if (op != ImGuizmo::TRANSLATE) {
                op = ImGuizmo::TRANSLATE;
                rv = true;
            }
        }
        ui::DrawTooltipIfItemHovered("Translate", "Make the 3D manipulation gizmos translate things (hotkey: G)");
        ui::PopStyleColor(std::exchange(num_colors_pushed, 0));
        ui::SameLine();
    }

    if (can_rotate) {
        if (op == ImGuizmo::ROTATE) {
            ui::PushStyleColor(ImGuiCol_Button, Color::muted_blue());
            ++num_colors_pushed;
        }
        if (ui::Button(ICON_FA_REDO)) {
            if (op != ImGuizmo::ROTATE) {
                op = ImGuizmo::ROTATE;
                rv = true;
            }
        }
        ui::DrawTooltipIfItemHovered("Rotate", "Make the 3D manipulation gizmos rotate things (hotkey: R)");
        ui::PopStyleColor(std::exchange(num_colors_pushed, 0));
        ui::SameLine();
    }

    if (can_scale) {
        if (op == ImGuizmo::SCALE) {
            ui::PushStyleColor(ImGuiCol_Button, Color::muted_blue());
            ++num_colors_pushed;
        }
        if (ui::Button(ICON_FA_EXPAND_ARROWS_ALT)) {
            if (op != ImGuizmo::SCALE) {
                op = ImGuizmo::SCALE;
                rv = true;
            }
        }
        ui::DrawTooltipIfItemHovered("Scale", "Make the 3D manipulation gizmos scale things (hotkey: S)");
        ui::PopStyleColor(std::exchange(num_colors_pushed, 0));
        ui::SameLine();
    }

    ui::PopStyleVar(2);

    return rv;
}

bool osc::UpdateImguizmoStateFromKeyboard(
    ImGuizmo::OPERATION& op,
    ImGuizmo::MODE& mode)
{
    bool const shift_down = ui::IsShiftDown();
    bool const ctrl_or_super_down = ui::IsCtrlOrSuperDown();

    if (shift_down or ctrl_or_super_down) {
        return false;  // assume the user is doing some other action
    }
    else if (ui::IsKeyPressed(ImGuiKey_R)) {

        // R: set manipulation mode to "rotate"
        if (op == ImGuizmo::ROTATE) {
            mode = mode == ImGuizmo::LOCAL ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
        }
        op = ImGuizmo::ROTATE;
        return true;
    }
    else if (ui::IsKeyPressed(ImGuiKey_G)) {

        // G: set manipulation mode to "grab" (translate)
        if (op == ImGuizmo::TRANSLATE) {
            mode = mode == ImGuizmo::LOCAL ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
        }
        op = ImGuizmo::TRANSLATE;
        return true;
}
    else if (ui::IsKeyPressed(ImGuiKey_S)) {

        // S: set manipulation mode to "scale"
        if (op == ImGuizmo::SCALE) {
            mode = mode == ImGuizmo::LOCAL ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
        }
        op = ImGuizmo::SCALE;
        return true;
    }
    else {
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
