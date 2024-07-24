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
namespace plot = osc::ui::plot;

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

// `ui::plot::` helpers
namespace
{
    constexpr ImPlotFlags to_ImPlotFlags(plot::PlotFlags flags)
    {
        static_assert(cpp23::to_underlying(plot::PlotFlags::NoTitle) == ImPlotFlags_NoTitle);
        static_assert(cpp23::to_underlying(plot::PlotFlags::NoLegend) == ImPlotFlags_NoLegend);
        static_assert(cpp23::to_underlying(plot::PlotFlags::NoMenus) == ImPlotFlags_NoMenus);
        static_assert(cpp23::to_underlying(plot::PlotFlags::NoBoxSelect) == ImPlotFlags_NoBoxSelect);
        static_assert(cpp23::to_underlying(plot::PlotFlags::NoFrame) == ImPlotFlags_NoFrame);
        static_assert(cpp23::to_underlying(plot::PlotFlags::NoInputs) == ImPlotFlags_NoInputs);
        return static_cast<ImPlotFlags>(flags);
    }

    constexpr ImPlotStyleVar to_ImPlotStyleVar(plot::StyleVar var)
    {
        static_assert(num_options<plot::StyleVar>() == 4);

        switch (var) {
        case plot::StyleVar::FitPadding:        return ImPlotStyleVar_FitPadding;
        case plot::StyleVar::PlotPadding:       return ImPlotStyleVar_PlotPadding;
        case plot::StyleVar::PlotBorderSize:    return ImPlotStyleVar_PlotBorderSize;
        case plot::StyleVar::AnnotationPadding: return ImPlotStyleVar_AnnotationPadding;
        default:                                return ImPlotStyleVar_PlotPadding;  // shouldn't happen
        }
    }

    constexpr ImPlotCol to_ImPlotCol(plot::ColorVar var)
    {
        static_assert(num_options<plot::ColorVar>() == 2);

        switch (var) {
        case plot::ColorVar::Line:           return ImPlotCol_Line;
        case plot::ColorVar::PlotBackground: return ImPlotCol_PlotBg;
        default:                             return ImPlotCol_Line;  // shouldn't happen
        }
    }

    constexpr ImAxis to_ImAxis(plot::Axis axis)
    {
        static_assert(num_options<plot::Axis>() == 2);

        switch (axis) {
        case plot::Axis::X1: return ImAxis_X1;
        case plot::Axis::Y1: return ImAxis_Y1;
        default:             return ImAxis_X1;  // shouldn't happen
        }
    }

    constexpr ImPlotAxisFlags to_ImPlotAxisFlags(plot::AxisFlags flags)
    {
        static_assert(cpp23::to_underlying(plot::AxisFlags::None) == ImPlotAxisFlags_None);
        static_assert(cpp23::to_underlying(plot::AxisFlags::NoLabel) == ImPlotAxisFlags_NoLabel);
        static_assert(cpp23::to_underlying(plot::AxisFlags::NoGridLines) == ImPlotAxisFlags_NoGridLines);
        static_assert(cpp23::to_underlying(plot::AxisFlags::NoTickMarks) == ImPlotAxisFlags_NoTickMarks);
        static_assert(cpp23::to_underlying(plot::AxisFlags::NoTickLabels) == ImPlotAxisFlags_NoTickLabels);
        static_assert(cpp23::to_underlying(plot::AxisFlags::NoMenus) == ImPlotAxisFlags_NoMenus);
        static_assert(cpp23::to_underlying(plot::AxisFlags::AutoFit) == ImPlotAxisFlags_AutoFit);
        static_assert(cpp23::to_underlying(plot::AxisFlags::LockMin) == ImPlotAxisFlags_LockMin);
        static_assert(cpp23::to_underlying(plot::AxisFlags::LockMax) == ImPlotAxisFlags_LockMax);
        static_assert(cpp23::to_underlying(plot::AxisFlags::Lock) == ImPlotAxisFlags_Lock);
        static_assert(cpp23::to_underlying(plot::AxisFlags::NoDecorations) == ImPlotAxisFlags_NoDecorations);

        return static_cast<ImPlotAxisFlags>(flags);
    }

    constexpr ImPlotCond to_ImPlotCond(plot::Condition condition)
    {
        static_assert(num_options<plot::Condition>() == 2);
        switch (condition) {
        case plot::Condition::Always: return ImPlotCond_Always;
        case plot::Condition::Once:   return ImPlotCond_Once;
        default:                      return ImPlotCond_Once;  // shouldn't happen
        }
    }

    constexpr ImPlotMarker to_ImPlotMarker(plot::MarkerType marker_type)
    {
        static_assert(num_options<plot::MarkerType>() == 2);
        switch (marker_type) {
        case plot::MarkerType::None:   return ImPlotMarker_None;
        case plot::MarkerType::Circle: return ImPlotMarker_Circle;
        default:                       return ImPlotMarker_None;  // shouldn't happen
        }
    }

    constexpr ImPlotDragToolFlags to_ImPlotDragToolFlags(plot::DragToolFlags flags)
    {
        static_assert(cpp23::to_underlying(plot::DragToolFlags::None) == ImPlotDragToolFlags_None);
        static_assert(cpp23::to_underlying(plot::DragToolFlags::NoFit) == ImPlotDragToolFlags_NoFit);
        static_assert(cpp23::to_underlying(plot::DragToolFlags::NoInputs) == ImPlotDragToolFlags_NoInputs);
        return static_cast<ImPlotDigitalFlags>(flags);
    }

    constexpr ImPlotLocation to_ImPlotLocation(plot::Location location)
    {
        static_assert(num_options<plot::Location>() == 9);
        switch (location) {
        case plot::Location::Center:    return ImPlotLocation_Center;
        case plot::Location::North:     return ImPlotLocation_North;
        case plot::Location::NorthEast: return ImPlotLocation_NorthEast;
        case plot::Location::East:      return ImPlotLocation_East;
        case plot::Location::SouthEast: return ImPlotLocation_SouthEast;
        case plot::Location::South:     return ImPlotLocation_South;
        case plot::Location::SouthWest: return ImPlotLocation_SouthWest;
        case plot::Location::West:      return ImPlotLocation_West;
        case plot::Location::NorthWest: return ImPlotLocation_NorthWest;
        default:                        return ImPlotLocation_Center;  // shouldn't happen
        }
    }

    constexpr ImPlotLegendFlags to_ImPlotLegendFlags(plot::LegendFlags flags)
    {
        static_assert(cpp23::to_underlying(plot::LegendFlags::None) == ImPlotLegendFlags_None);
        static_assert(cpp23::to_underlying(plot::LegendFlags::Outside) == ImPlotLegendFlags_Outside);
        return static_cast<ImPlotLegendFlags>(flags);
    }
}

void osc::ui::plot::show_demo_panel()
{
    ImPlot::ShowDemoWindow();
}

bool osc::ui::plot::begin(CStringView title, Vec2 size, PlotFlags flags)
{
    return ImPlot::BeginPlot(title.c_str(), size, to_ImPlotFlags(flags));
}

void osc::ui::plot::end()
{
    ImPlot::EndPlot();
}

void osc::ui::plot::push_style_var(StyleVar var, float value)
{
    ImPlot::PushStyleVar(to_ImPlotStyleVar(var), value);
}

void osc::ui::plot::push_style_var(StyleVar var, Vec2 value)
{
    ImPlot::PushStyleVar(to_ImPlotStyleVar(var), value);
}

void osc::ui::plot::pop_style_var(int count)
{
    ImPlot::PopStyleVar(count);
}

void osc::ui::plot::push_style_color(ColorVar var, const Color& color)
{
    ImPlot::PushStyleColor(to_ImPlotCol(var), color);
}

void osc::ui::plot::pop_style_color(int count)
{
    ImPlot::PopStyleColor(count);
}

void osc::ui::plot::setup_axis(Axis axis, std::optional<CStringView> label, AxisFlags flags)
{
    ImPlot::SetupAxis(to_ImAxis(axis), label ? label->c_str() : nullptr, to_ImPlotAxisFlags(flags));
}

void osc::ui::plot::setup_axes(CStringView x_label, CStringView y_label, AxisFlags x_flags, AxisFlags y_flags)
{
    ImPlot::SetupAxes(x_label.c_str(), y_label.c_str(), to_ImPlotAxisFlags(x_flags), to_ImPlotAxisFlags(y_flags));
}

void osc::ui::plot::setup_axis_limits(Axis axis, ClosedInterval<float> data_range, float padding_percentage, Condition condition)
{
    // apply padding
    data_range = expand_by_absolute_amount(data_range, padding_percentage * data_range.half_length());

    // apply absolute padding in the edge-case where the data is constant
    if (equal_within_scaled_epsilon(data_range.lower, data_range.upper)) {
        data_range = expand_by_absolute_amount(data_range, 0.5f);
    }

    ImPlot::SetupAxisLimits(to_ImAxis(axis), data_range.lower, data_range.upper, to_ImPlotCond(condition));
}

void osc::ui::plot::setup_finish()
{
    ImPlot::SetupFinish();
}

void osc::ui::plot::set_next_marker_style(
    MarkerType marker_type,
    std::optional<float> size,
    std::optional<Color> fill,
    std::optional<float> weight,
    std::optional<Color> outline)
{
    ImPlot::SetNextMarkerStyle(
        to_ImPlotMarker(marker_type),
        size ? *size : IMPLOT_AUTO,
        fill ? ImVec4{*fill} : IMPLOT_AUTO_COL,
        weight ? *weight : IMPLOT_AUTO,
        outline ? ImVec4{*outline} : IMPLOT_AUTO_COL
    );
}

void osc::ui::plot::plot_line(CStringView name, std::span<const Vec2> points)
{
    ImPlot::PlotLine(
        name.c_str(),
        points.empty() ? nullptr : &points.front().x,
        points.empty() ? nullptr : &points.front().y,
        static_cast<int>(points.size()),
        0,
        0,
        sizeof(Vec2)
    );
}

void osc::ui::plot::plot_line(CStringView name, std::span<const float> points)
{
    ImPlot::PlotLine(name.c_str(), points.data(), static_cast<int>(points.size()));
}

Rect osc::ui::plot::get_plot_screen_rect()
{
    const Vec2 top_left = ImPlot::GetPlotPos();
    return {top_left, top_left + Vec2{ImPlot::GetPlotSize()}};
}

void osc::ui::plot::draw_annotation_v(Vec2 location_dataspace, const Color& color, Vec2 pixel_offset, bool clamp, const char* fmt, va_list args)
{
    ImPlot::AnnotationV(location_dataspace.x, location_dataspace.y, color, pixel_offset, clamp, fmt, args);
}

bool osc::ui::plot::drag_point(int id, Vec2d* location, const Color& color, float size, DragToolFlags flags)
{
    return ImPlot::DragPoint(id, &location->x, &location->y, color, size, to_ImPlotDragToolFlags(flags));
}

bool osc::ui::plot::drag_line_x(int id, double* x, const Color& color, float thickness, DragToolFlags flags)
{
    return ImPlot::DragLineX(id, x, color, thickness, to_ImPlotDragToolFlags(flags));
}

bool osc::ui::plot::drag_line_y(int id, double* y, const Color& color, float thickness, DragToolFlags flags)
{
    return ImPlot::DragLineY(id, y, color, thickness, to_ImPlotDragToolFlags(flags));
}

void osc::ui::plot::tag_x(double x, const Color& color, bool round)
{
    ImPlot::TagX(x, color, round);
}

bool osc::ui::plot::is_plot_hovered()
{
    return ImPlot::IsPlotHovered();
}

Vec2 osc::ui::plot::get_plot_mouse_pos()
{
    const auto pos = ImPlot::GetPlotMousePos();
    return Vec2{pos.x, pos.y};
}

Vec2 osc::ui::plot::get_plot_mouse_pos(Axis x_axis, Axis y_axis)
{
    const auto pos = ImPlot::GetPlotMousePos(to_ImAxis(x_axis), to_ImAxis(y_axis));
    return Vec2{pos.x, pos.y};
}

void osc::ui::plot::setup_legend(Location location, LegendFlags flags)
{
    ImPlot::SetupLegend(to_ImPlotLocation(location), to_ImPlotLegendFlags(flags));
}

bool osc::ui::plot::begin_legend_popup(CStringView label_id, ImGuiMouseButton mouse_button)
{
    return ImPlot::BeginLegendPopup(label_id.c_str(), mouse_button);
}

void osc::ui::plot::end_legend_popup()
{
    ImPlot::EndLegendPopup();
}
