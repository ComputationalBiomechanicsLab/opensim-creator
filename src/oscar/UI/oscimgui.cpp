#include "oscimgui.h"

#include <implot.h>
#include <oscar/Maths/ClosedInterval.h>
#include <oscar/Maths/CommonFunctions.h>
#include <oscar/Maths/EulerAngles.h>
#include <oscar/Maths/RectFunctions.h>
#include <oscar/Maths/MatFunctions.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/Quat.h>
#include <oscar/Maths/Transform.h>
#include <oscar/Maths/VecFunctions.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/Utils/EnumHelpers.h>
#include <oscar/Utils/ScopeGuard.h>
#include <oscar/Utils/UID.h>

#include <IconsFontAwesome5.h>
#include <ImGuizmo.h>

#include <algorithm>
#include <functional>
#include <ranges>

using namespace osc;
namespace rgs = std::ranges;

namespace
{
    ImGuizmo::OPERATION to_imguizmo_operation(ui::GizmoOperation op)
    {
        static_assert(num_options<ui::GizmoOperation>() == 3);
        switch (op) {
        case ui::GizmoOperation::Scale:     return ImGuizmo::OPERATION::SCALE;
        case ui::GizmoOperation::Rotate:    return ImGuizmo::OPERATION::ROTATE;
        case ui::GizmoOperation::Translate: return ImGuizmo::OPERATION::TRANSLATE;
        default:                            return ImGuizmo::OPERATION::TRANSLATE;
        }
    }

    ImGuizmo::MODE to_imguizmo_mode(ui::GizmoMode mode)
    {
        static_assert(num_options<ui::GizmoMode>() == 2);
        switch (mode) {
        case ui::GizmoMode::Local: return ImGuizmo::MODE::LOCAL;
        case ui::GizmoMode::World: return ImGuizmo::MODE::WORLD;
        default:                   return ImGuizmo::MODE::WORLD;
        }
    }
}

bool osc::ui::draw_gizmo_mode_selector(Gizmo& gizmo)
{
    GizmoMode mode = gizmo.mode();
    if (draw_gizmo_mode_selector(mode)) {
        gizmo.set_mode(mode);
        return true;
    }
    return false;
}

bool osc::ui::draw_gizmo_mode_selector(GizmoMode& mode)
{
    constexpr auto mode_labels = std::to_array({ "local", "global" });
    constexpr auto modes = std::to_array<GizmoMode, 2>({ GizmoMode::Local, GizmoMode::World });

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
    Gizmo& gizmo,
    bool can_translate,
    bool can_rotate,
    bool can_scale)
{
    GizmoOperation op = gizmo.operation();
    if (draw_gizmo_op_selector(op, can_translate, can_rotate, can_scale)) {
        gizmo.set_operation(op);
        return true;
    }
    return false;
}

bool osc::ui::draw_gizmo_op_selector(
    GizmoOperation& op,
    bool can_translate,
    bool can_rotate,
    bool can_scale)
{
    bool rv = false;

    ui::push_style_var(ImGuiStyleVar_ItemSpacing, {0.0f, 0.0f});
    ui::push_style_var(ImGuiStyleVar_FrameRounding, 0.0f);
    int num_colors_pushed = 0;

    if (can_translate) {
        if (op == GizmoOperation::Translate) {
            ui::push_style_color(ImGuiCol_Button, Color::muted_blue());
            ++num_colors_pushed;
        }
        if (ui::draw_button(ICON_FA_ARROWS_ALT)) {
            if (op != GizmoOperation::Translate) {
                op = GizmoOperation::Translate;
                rv = true;
            }
        }
        ui::draw_tooltip_if_item_hovered("Translate", "Make the 3D manipulation gizmos translate things (hotkey: G)");
        ui::pop_style_color(std::exchange(num_colors_pushed, 0));
        ui::same_line();
    }

    if (can_rotate) {
        if (op == GizmoOperation::Rotate) {
            ui::push_style_color(ImGuiCol_Button, Color::muted_blue());
            ++num_colors_pushed;
        }
        if (ui::draw_button(ICON_FA_REDO)) {
            if (op != GizmoOperation::Rotate) {
                op = GizmoOperation::Rotate;
                rv = true;
            }
        }
        ui::draw_tooltip_if_item_hovered("Rotate", "Make the 3D manipulation gizmos rotate things (hotkey: R)");
        ui::pop_style_color(std::exchange(num_colors_pushed, 0));
        ui::same_line();
    }

    if (can_scale) {
        if (op == GizmoOperation::Scale) {
            ui::push_style_color(ImGuiCol_Button, Color::muted_blue());
            ++num_colors_pushed;
        }
        if (ui::draw_button(ICON_FA_EXPAND_ARROWS_ALT)) {
            if (op != GizmoOperation::Scale) {
                op = GizmoOperation::Scale;
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

std::optional<Transform> osc::ui::Gizmo::draw(
    Mat4& model_matrix,
    const Mat4& view_matrix,
    const Mat4& projection_matrix,
    const Rect& screenspace_rect)
{
    // important: necessary for multi-viewport gizmos
    // also important: don't use ui::get_id(), because it uses an ID stack and we might want to know if "isover" etc. is true outside of a window
    ImGuizmo::SetID(static_cast<int>(std::hash<UID>{}(id_)));
    const ScopeGuard g{[]() { ImGuizmo::SetID(-1); }};

    // update last-frame cache
    was_using_last_frame_ = ImGuizmo::IsUsing();

    ImGuizmo::SetRect(
        screenspace_rect.p1.x,
        screenspace_rect.p1.y,
        dimensions_of(screenspace_rect).x,
        dimensions_of(screenspace_rect).y
    );
    ImGuizmo::SetDrawlist(ui::get_panel_draw_list());
    ImGuizmo::AllowAxisFlip(false);  // user's didn't like this feature in UX sessions

    // use rotation from the parent, translation from station
    Mat4 delta_matrix;

    // ensure style matches OSC requirements
    {
        ImGuizmo::Style& style = ImGuizmo::GetStyle();
        style.TranslationLineThickness = 5.0f;
        style.TranslationLineArrowSize = 8.0f;
        style.RotationLineThickness = 5.0f;
        style.RotationOuterLineThickness = 7.0f;
        style.ScaleLineThickness = 5.0f;
        style.ScaleLineCircleSize = 8.0f;
    }

    const bool gizmo_was_manipulated_by_user = ImGuizmo::Manipulate(
        value_ptr(view_matrix),
        value_ptr(projection_matrix),
        to_imguizmo_operation(operation_),
        to_imguizmo_mode(mode_),
        value_ptr(model_matrix),
        value_ptr(delta_matrix),
        nullptr,
        nullptr,
        nullptr
    );

    if (not gizmo_was_manipulated_by_user) {
        return std::nullopt;  // user is not interacting, so no changes to apply
    }
    // else: figure out the local-space transform

    // decompose the additional transformation into component parts
    Vec3 world_translation{};
    Vec3 world_rotation_in_degrees{};
    Vec3 world_scale{};
    ImGuizmo::DecomposeMatrixToComponents(
        value_ptr(delta_matrix),
        value_ptr(world_translation),
        value_ptr(world_rotation_in_degrees),
        value_ptr(world_scale)
    );

    return Transform{
        .scale = world_scale,
        .rotation = to_worldspace_rotation_quat(EulerAnglesIn<Degrees>{world_rotation_in_degrees}),
        .position = world_translation,
    };
}

bool osc::ui::Gizmo::is_using() const
{
    ImGuizmo::SetID(static_cast<int>(std::hash<UID>{}(id_)));
    const bool rv = ImGuizmo::IsUsing();
    ImGuizmo::SetID(-1);
    return rv;
}

bool osc::ui::Gizmo::is_over() const
{
    ImGuizmo::SetID(static_cast<int>(std::hash<UID>{}(id_)));
    const bool rv = ImGuizmo::IsOver();
    ImGuizmo::SetID(-1);
    return rv;
}

bool osc::ui::Gizmo::handle_keyboard_inputs()
{
    bool const shift_down = ui::is_shift_down();
    bool const ctrl_or_super_down = ui::is_ctrl_or_super_down();

    if (shift_down or ctrl_or_super_down) {
        return false;  // assume the user is doing some other action
    }
    else if (ui::is_key_pressed(ImGuiKey_R)) {

        // R: set manipulation mode to "rotate"
        if (operation_ == GizmoOperation::Rotate) {
            mode_ = mode_ == GizmoMode::Local ? GizmoMode::World : GizmoMode::Local;
        }
        operation_ = GizmoOperation::Rotate;
        return true;
    }
    else if (ui::is_key_pressed(ImGuiKey_G)) {

        // G: set manipulation mode to "grab" (translate)
        if (operation_ == GizmoOperation::Translate) {
            mode_ = mode_ == GizmoMode::Local ? GizmoMode::World : GizmoMode::Local;
        }
        operation_ = GizmoOperation::Translate;
        return true;
    }
    else if (ui::is_key_pressed(ImGuiKey_S)) {

        // S: set manipulation mode to "scale"
        if (operation_ == GizmoOperation::Scale) {
            mode_ = mode_ == GizmoMode::Local ? GizmoMode::World : GizmoMode::Local;
        }
        operation_ = GizmoOperation::Scale;
        return true;
    }
    else {
        return false;
    }
}

bool osc::ui::plot::begin(CStringView title, Vec2 size, ImPlotFlags flags)
{
    return ImPlot::BeginPlot(title.c_str(), size, flags);
}

void osc::ui::plot::end()
{
    ImPlot::EndPlot();
}

void osc::ui::plot::push_style_color(ImPlotCol idx, const Color& color)
{
    ImPlot::PushStyleColor(idx, Vec4{color});
}

void osc::ui::plot::pop_style_color(int count)
{
    ImPlot::PopStyleColor(count);
}

void osc::ui::plot::setup_axes(CStringView x_label, CStringView y_label, ImPlotAxisFlags x_flags, ImPlotAxisFlags y_flags)
{
    ImPlot::SetupAxes(x_label.c_str(), y_label.c_str(), x_flags, y_flags);
}

void osc::ui::plot::setup_finish()
{
    ImPlot::SetupFinish();
}

void osc::ui::plot::setup_axis_limits(ImAxis axis, ClosedInterval<float> data_range, float padding_percentage, ImPlotCond cond)
{
    // apply padding
    data_range = expand_by_absolute_amount(data_range, padding_percentage * data_range.half_length());

    // apply absolute padding in the edge-case where the data is constant
    if (equal_within_scaled_epsilon(data_range.lower, data_range.upper)) {
        data_range = expand_by_absolute_amount(data_range, 0.5f);
    }

    ImPlot::SetupAxisLimits(axis, data_range.lower, data_range.upper, cond);
}

void osc::ui::plot::plot_line(CStringView name, std::span<const Vec2> points, ImPlotLineFlags flags)
{
    if (points.empty()) {
        return;
    }

    ImPlot::PlotLine(name.c_str(), &points.front().x, &points.front().y, static_cast<int>(points.size()), flags, 0, sizeof(Vec2));
}
