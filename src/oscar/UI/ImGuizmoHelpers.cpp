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

bool osc::ui::draw_gizmo_mode_selector(ImGuizmo::MODE& mode)
{
    constexpr auto mode_labels = std::to_array({ "local", "global" });
    constexpr auto modes = std::to_array<ImGuizmo::MODE, 2>({ ImGuizmo::LOCAL, ImGuizmo::WORLD });

    bool rv = false;
    int current_mode = static_cast<int>(std::distance(rgs::begin(modes), rgs::find(modes, mode)));
    ui::push_style_var(ImGuiStyleVar_FrameRounding, 0.0f);
    ui::set_next_item_width(ui::calc_text_size(mode_labels[0]).x + 40.0f);
    if (ui::draw_combobox("##modeselect", &current_mode, mode_labels.data(), static_cast<int>(mode_labels.size()))) {
        mode = modes.at(static_cast<size_t>(current_mode));
        rv = true;
    }
    ui::pop_style_var();
    constexpr CStringView tooltip_title = "Manipulation coordinate system";
    constexpr CStringView tooltip_description = "This affects whether manipulations (such as the arrow gizmos that you can use to translate things) are performed relative to the global coordinate system or the selection's (local) one. Local manipulations can be handy when translating/rotating something that's already rotated.";
    ui::draw_tooltip_if_item_hovered(tooltip_title, tooltip_description);

    return rv;
}

bool osc::ui::draw_gizmo_op_selector(
    ImGuizmo::OPERATION& op,
    bool can_translate,
    bool can_rotate,
    bool can_scale)
{
    bool rv = false;

    ui::push_style_var(ImGuiStyleVar_ItemSpacing, {0.0f, 0.0f});
    ui::push_style_var(ImGuiStyleVar_FrameRounding, 0.0f);
    int num_colors_pushed = 0;

    if (can_translate) {
        if (op == ImGuizmo::TRANSLATE) {
            ui::push_style_color(ImGuiCol_Button, Color::muted_blue());
            ++num_colors_pushed;
        }
        if (ui::draw_button(ICON_FA_ARROWS_ALT)) {
            if (op != ImGuizmo::TRANSLATE) {
                op = ImGuizmo::TRANSLATE;
                rv = true;
            }
        }
        ui::draw_tooltip_if_item_hovered("Translate", "Make the 3D manipulation gizmos translate things (hotkey: G)");
        ui::pop_style_color(std::exchange(num_colors_pushed, 0));
        ui::same_line();
    }

    if (can_rotate) {
        if (op == ImGuizmo::ROTATE) {
            ui::push_style_color(ImGuiCol_Button, Color::muted_blue());
            ++num_colors_pushed;
        }
        if (ui::draw_button(ICON_FA_REDO)) {
            if (op != ImGuizmo::ROTATE) {
                op = ImGuizmo::ROTATE;
                rv = true;
            }
        }
        ui::draw_tooltip_if_item_hovered("Rotate", "Make the 3D manipulation gizmos rotate things (hotkey: R)");
        ui::pop_style_color(std::exchange(num_colors_pushed, 0));
        ui::same_line();
    }

    if (can_scale) {
        if (op == ImGuizmo::SCALE) {
            ui::push_style_color(ImGuiCol_Button, Color::muted_blue());
            ++num_colors_pushed;
        }
        if (ui::draw_button(ICON_FA_EXPAND_ARROWS_ALT)) {
            if (op != ImGuizmo::SCALE) {
                op = ImGuizmo::SCALE;
                rv = true;
            }
        }
        ui::draw_tooltip_if_item_hovered("Scale", "Make the 3D manipulation gizmos scale things (hotkey: S)");
        ui::pop_style_color(std::exchange(num_colors_pushed, 0));
        ui::same_line();
    }

    ui::pop_style_var(2);

    return rv;
}

bool osc::ui::update_gizmo_state_from_keyboard(
    ImGuizmo::OPERATION& op,
    ImGuizmo::MODE& mode)
{
    bool const shift_down = ui::is_shift_down();
    bool const ctrl_or_super_down = ui::is_ctrl_or_super_down();

    if (shift_down or ctrl_or_super_down) {
        return false;  // assume the user is doing some other action
    }
    else if (ui::is_key_pressed(ImGuiKey_R)) {

        // R: set manipulation mode to "rotate"
        if (op == ImGuizmo::ROTATE) {
            mode = mode == ImGuizmo::LOCAL ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
        }
        op = ImGuizmo::ROTATE;
        return true;
    }
    else if (ui::is_key_pressed(ImGuiKey_G)) {

        // G: set manipulation mode to "grab" (translate)
        if (op == ImGuizmo::TRANSLATE) {
            mode = mode == ImGuizmo::LOCAL ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
        }
        op = ImGuizmo::TRANSLATE;
        return true;
}
    else if (ui::is_key_pressed(ImGuiKey_S)) {

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

void osc::ui::set_gizmo_style_to_osc_standard()
{
    ImGuizmo::Style& style = ImGuizmo::GetStyle();
    style.TranslationLineThickness = 5.0f;
    style.TranslationLineArrowSize = 8.0f;
    style.RotationLineThickness = 5.0f;
    style.RotationOuterLineThickness = 7.0f;
    style.ScaleLineThickness = 5.0f;
    style.ScaleLineCircleSize = 8.0f;
}
