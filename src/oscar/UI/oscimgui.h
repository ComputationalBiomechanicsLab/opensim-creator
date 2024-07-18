#pragma once

#define IMGUI_USER_CONFIG <oscar/UI/oscimgui_config.h>

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui/misc/cpp/imgui_stdlib.h>
#include <implot.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Maths/EulerAngles.h>
#include <oscar/Maths/Mat4.h>
#include <oscar/Maths/Rect.h>
#include <oscar/Maths/Transform.h>
#include <oscar/Maths/Vec.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/UID.h>

#include <cstddef>
#include <optional>
#include <utility>

namespace osc::ui
{
    inline void align_text_to_frame_padding()
    {
        ImGui::AlignTextToFramePadding();
    }

    inline void draw_text(CStringView sv)
    {
        ImGui::TextUnformatted(sv.c_str(), sv.c_str() + sv.size());
    }

    inline void draw_text(const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        ImGui::TextV(fmt, args);
        va_end(args);
    }

    inline void draw_text_disabled(CStringView sv)
    {
        ImGui::TextDisabled("%s", sv.c_str());
    }

    inline void draw_text_disabled(const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        ImGui::TextDisabledV(fmt, args);
        va_end(args);
    }

    inline void draw_text_wrapped(CStringView sv)
    {
        ImGui::TextWrapped("%s", sv.c_str());
    }

    inline void draw_text_wrapped(const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        ImGui::TextWrappedV(fmt, args);
        va_end(args);
    }

    inline void draw_text_unformatted(CStringView sv)
    {
        ImGui::TextUnformatted(sv.c_str(), sv.c_str() + sv.size());
    }

    inline void draw_bullet_point()
    {
        ImGui::Bullet();
    }

    inline void draw_text_bullet_pointed(CStringView str)
    {
        ImGui::BulletText("%s", str.c_str());
    }

    inline bool draw_tree_node_ex(CStringView label, ImGuiTreeNodeFlags flags = 0)
    {
        return ImGui::TreeNodeEx(label.c_str(), flags);
    }

    inline float get_tree_node_to_label_spacing()
    {
        return ImGui::GetTreeNodeToLabelSpacing();
    }

    inline void tree_pop()
    {
        ImGui::TreePop();
    }

    inline void draw_progress_bar(float fraction)
    {
        ImGui::ProgressBar(fraction);
    }

    inline bool begin_menu(CStringView sv, bool enabled = true)
    {
        return ImGui::BeginMenu(sv.c_str(), enabled);
    }

    inline void end_menu()
    {
        return ImGui::EndMenu();
    }

    inline bool draw_menu_item(
        CStringView label,
        CStringView shortcut = {},
        bool selected = false,
        bool enabled = true)
    {
        return ImGui::MenuItem(label.c_str(), shortcut.empty() ? nullptr : shortcut.c_str(), selected, enabled);
    }

    inline bool draw_menu_item(
        CStringView label,
        CStringView shortcut,
        bool* p_selected,
        bool enabled = true)
    {
        return ImGui::MenuItem(label.c_str(), shortcut.empty() ? nullptr : shortcut.c_str(), p_selected, enabled);
    }

    inline bool begin_tab_bar(CStringView str_id)
    {
        return ImGui::BeginTabBar(str_id.c_str());
    }

    inline void end_tab_bar()
    {
        ImGui::EndTabBar();
    }

    inline bool begin_tab_item(CStringView label, bool* p_open = nullptr, ImGuiTabItemFlags flags = 0)
    {
        return ImGui::BeginTabItem(label.c_str(), p_open, flags);
    }

    inline void end_tab_item()
    {
        ImGui::EndTabItem();
    }

    inline bool draw_tab_item_button(CStringView label)
    {
        return ImGui::TabItemButton(label.c_str());
    }

    inline void set_num_columns(int count = 1, const char* id = nullptr, bool border = true)
    {
        ImGui::Columns(count, id, border);
    }

    inline float get_column_width(int column_index = -1)
    {
        return ImGui::GetColumnWidth(column_index);
    }

    inline void next_column()
    {
        ImGui::NextColumn();
    }

    inline void same_line(float offset_from_start_x = 0.0f, float spacing = -1.0f)
    {
        ImGui::SameLine(offset_from_start_x, spacing);
    }

    inline bool is_mouse_clicked(ImGuiMouseButton button, bool repeat = false)
    {
        return ImGui::IsMouseClicked(button, repeat);
    }

    inline bool is_mouse_clicked(ImGuiMouseButton button, ImGuiID owner_id, ImGuiInputFlags flags = 0)
    {
        return ImGui::IsMouseClicked(button, owner_id, flags);
    }

    inline bool is_mouse_released(ImGuiMouseButton button)
    {
        return ImGui::IsMouseReleased(button);
    }

    inline bool is_mouse_down(ImGuiMouseButton button)
    {
        return ImGui::IsMouseDown(button);
    }

    inline bool is_mouse_dragging(ImGuiMouseButton button, float lock_threshold = -1.0f)
    {
        return ImGui::IsMouseDragging(button, lock_threshold);
    }

    inline bool draw_selectable(CStringView label, bool* p_selected, ImGuiSelectableFlags flags = 0, const Vec2& size = {})
    {
        return ImGui::Selectable(label.c_str(), p_selected, flags, size);
    }

    inline bool draw_selectable(CStringView label, bool selected = false, ImGuiSelectableFlags flags = 0, const Vec2& size = {})
    {
        return ImGui::Selectable(label.c_str(), selected, flags, size);
    }

    inline bool draw_checkbox(CStringView label, bool* v)
    {
        return ImGui::Checkbox(label.c_str(), v);
    }

    inline bool draw_checkbox_flags(CStringView label, int* flags, int flags_value)
    {
        return ImGui::CheckboxFlags(label.c_str(), flags, flags_value);
    }

    inline bool draw_checkbox_flags(CStringView label, unsigned int* flags, unsigned int flags_value)
    {
        return ImGui::CheckboxFlags(label.c_str(), flags, flags_value);
    }

    inline bool draw_float_slider(CStringView label, float* v, float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0)
    {
        return ImGui::SliderFloat(label.c_str(), v, v_min, v_max, format, flags);
    }

    inline bool draw_scalar_input(CStringView label, ImGuiDataType data_type, void* p_data, const void* p_step = NULL, const void* p_step_fast = NULL, const char* format = NULL, ImGuiInputTextFlags flags = 0)
    {
        return ImGui::InputScalar(label.c_str(), data_type, p_data, p_step, p_step_fast, format, flags);
    }

    inline bool draw_int_input(CStringView label, int* v, int step = 1, int step_fast = 100, ImGuiInputTextFlags flags = 0)
    {
        return ImGui::InputInt(label.c_str(), v, step, step_fast, flags);
    }

    inline bool draw_double_input(CStringView label, double* v, double step = 0.0, double step_fast = 0.0, const char* format = "%.6f", ImGuiInputTextFlags flags = 0)
    {
        return ImGui::InputDouble(label.c_str(), v, step, step_fast, format, flags);
    }

    inline bool draw_float_input(CStringView label, float* v, float step = 0.0f, float step_fast = 0.0f, const char* format = "%.3f", ImGuiInputTextFlags flags = 0)
    {
        return ImGui::InputFloat(label.c_str(), v, step, step_fast, format, flags);
    }

    inline bool draw_float3_input(CStringView label, float v[3], const char* format = "%.3f", ImGuiInputTextFlags flags = 0)
    {
        return ImGui::InputFloat3(label.c_str(), v, format, flags);
    }

    inline bool draw_vec3_input(CStringView label, Vec3& v, const char* format = "%.3f", ImGuiInputTextFlags flags = 0)
    {
        return ImGui::InputFloat3(label.c_str(), &v.x, format, flags);
    }

    inline bool draw_rgb_color_editor(CStringView label, Color& color)
    {
        return ImGui::ColorEdit3(label.c_str(), value_ptr(color));
    }

    inline bool draw_rgba_color_editor(CStringView label, Color& color)
    {
        return ImGui::ColorEdit4(label.c_str(), value_ptr(color));
    }

    inline bool draw_button(CStringView label, const Vec2& size = {})
    {
        return ImGui::Button(label.c_str(), size);
    }

    inline bool draw_small_button(CStringView label)
    {
        return ImGui::SmallButton(label.c_str());
    }

    inline bool draw_invisible_button(CStringView label, Vec2 size = {})
    {
        return ImGui::InvisibleButton(label.c_str(), size);
    }

    inline bool draw_radio_button(CStringView label, bool active)
    {
        return ImGui::RadioButton(label.c_str(), active);
    }

    inline bool draw_collapsing_header(CStringView label, ImGuiTreeNodeFlags flags = 0)
    {
        return ImGui::CollapsingHeader(label.c_str(), flags);
    }

    inline void draw_dummy(const Vec2& size)
    {
        ImGui::Dummy(size);
    }

    inline bool begin_combobox(CStringView label, CStringView preview_value, ImGuiComboFlags flags = 0)
    {
        return ImGui::BeginCombo(label.c_str(), preview_value.empty() ? nullptr : preview_value.c_str(), flags);
    }

    inline void end_combobox()
    {
        ImGui::EndCombo();
    }

    inline bool draw_combobox(CStringView label, int* current_item, const char* const items[], int items_count, int popup_max_height_in_items = -1)
    {
        return ImGui::Combo(label.c_str(), current_item, items, items_count, popup_max_height_in_items);
    }

    inline bool begin_listbox(CStringView label)
    {
        return ImGui::BeginListBox(label.c_str());
    }

    inline void end_listbox()
    {
        ImGui::EndListBox();
    }

    inline ImGuiViewport* get_main_viewport()
    {
        return ImGui::GetMainViewport();
    }

    inline ImGuiID enable_dockspace_over_viewport(const ImGuiViewport* viewport = NULL, ImGuiDockNodeFlags flags = 0, const ImGuiWindowClass* window_class = NULL)
    {
        return ImGui::DockSpaceOverViewport(0, viewport, flags, window_class);
    }

    inline bool begin_panel(CStringView name, bool* p_open = nullptr, ImGuiWindowFlags flags = 0)
    {
        return ImGui::Begin(name.c_str(), p_open, flags);
    }

    inline void end_panel()
    {
        ImGui::End();
    }

    inline bool begin_child_panel(CStringView str_id, const Vec2& size = {}, ImGuiChildFlags child_flags = 0, ImGuiWindowFlags panel_flags = 0)
    {
        return ImGui::BeginChild(str_id.c_str(), size, child_flags, panel_flags);
    }

    inline void end_child_panel()
    {
        ImGui::EndChild();
    }

    inline void close_current_popup()
    {
        ImGui::CloseCurrentPopup();
    }

    inline void set_tooltip(const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        ImGui::SetTooltipV(fmt, args);
        va_end(args);
    }

    inline void set_scroll_y_here()
    {
        ImGui::SetScrollHereY();
    }

    inline float get_frame_height()
    {
        return ImGui::GetFrameHeight();
    }

    inline Vec2 get_content_region_avail()
    {
        return ImGui::GetContentRegionAvail();
    }

    inline Vec2 get_cursor_start_pos()
    {
        return ImGui::GetCursorStartPos();
    }

    inline Vec2 get_cursor_pos()
    {
        return ImGui::GetCursorPos();
    }

    inline float get_cursor_pos_x()
    {
        return ImGui::GetCursorPosX();
    }

    inline void set_cursor_pos(Vec2 p)
    {
        ImGui::SetCursorPos(p);
    }

    inline void set_cursor_pos_x(float local_x)
    {
        ImGui::SetCursorPosX(local_x);
    }

    inline Vec2 get_cursor_screen_pos()
    {
        return ImGui::GetCursorScreenPos();
    }

    inline void set_cursor_screen_pos(Vec2 pos)
    {
        ImGui::SetCursorScreenPos(pos);
    }

    inline void set_next_panel_pos(Vec2 pos, ImGuiCond cond = 0, Vec2 pivot = {})
    {
        ImGui::SetNextWindowPos(pos, cond, pivot);
    }

    inline void set_next_panel_size(Vec2 size, ImGuiCond cond = 0)
    {
        ImGui::SetNextWindowSize(size, cond);
    }

    inline void set_next_panel_size_constraints(Vec2 size_min, Vec2 size_max)
    {
        ImGui::SetNextWindowSizeConstraints(size_min, size_max);
    }

    inline void set_next_panel_bg_alpha(float alpha)
    {
        ImGui::SetNextWindowBgAlpha(alpha);
    }

    inline bool is_panel_hovered(ImGuiHoveredFlags flags = 0)
    {
        return ImGui::IsWindowHovered(flags);
    }

    inline void begin_disabled(bool disabled = true)
    {
        ImGui::BeginDisabled(disabled);
    }

    inline void end_disabled()
    {
        ImGui::EndDisabled();
    }

    inline bool begin_tooltip_nowrap()
    {
        return ImGui::BeginTooltip();
    }

    inline void end_tooltip_nowrap()
    {
        ImGui::EndTooltip();
    }

    inline void push_id(UID id)
    {
        ImGui::PushID(static_cast<int>(id.get()));
    }

    inline void push_id(ptrdiff_t p)
    {
        ImGui::PushID(static_cast<int>(p));
    }

    inline void push_id(size_t i)
    {
        ImGui::PushID(static_cast<int>(i));
    }

    inline void push_id(int int_id)
    {
        ImGui::PushID(int_id);
    }

    inline void push_id(const void* ptr_id)
    {
        ImGui::PushID(ptr_id);
    }

    inline void push_id(CStringView str_id)
    {
        ImGui::PushID(str_id.data(), str_id.data() + str_id.size());
    }

    inline void pop_id()
    {
        ImGui::PopID();
    }

    inline ImGuiID get_id(CStringView str_id)
    {
        return ImGui::GetID(str_id.c_str());
    }

    inline ImGuiItemFlags get_item_flags()
    {
        return ImGui::GetItemFlags();
    }

    inline void set_next_item_size(Rect r)  // note: ImGui API assumes cursor is located at `p1` already
    {
        ImGui::ItemSize(ImRect{r.p1, r.p2});
    }

    inline bool add_item(Rect bounds, ImGuiID id)
    {
        return ImGui::ItemAdd(ImRect{bounds.p1, bounds.p2}, id);
    }

    inline bool is_item_hoverable(Rect bounds, ImGuiID id, ImGuiItemFlags item_flags)
    {
        return ImGui::ItemHoverable(ImRect{bounds.p1, bounds.p2}, id, item_flags);
    }

    inline void draw_separator()
    {
        ImGui::Separator();
    }

    inline void draw_separator(ImGuiSeparatorFlags flags)
    {
        ImGui::SeparatorEx(flags);
    }

    inline void start_new_line()
    {
        ImGui::NewLine();
    }

    inline void indent(float indent_w = 0.0f)
    {
        ImGui::Indent(indent_w);
    }

    inline void unindent(float indent_w = 0.0f)
    {
        ImGui::Unindent(indent_w);
    }

    inline void set_keyboard_focus_here()
    {
        ImGui::SetKeyboardFocusHere();
    }

    inline bool is_key_pressed(ImGuiKey key, bool repeat = true)
    {
        return ImGui::IsKeyPressed(key, repeat);
    }

    inline bool is_key_released(ImGuiKey key)
    {
        return ImGui::IsKeyReleased(key);
    }

    inline bool is_key_down(ImGuiKey key)
    {
        return ImGui::IsKeyDown(key);
    }

    inline ImGuiStyle& get_style()
    {
        return ImGui::GetStyle();
    }

    inline Color get_style_color(ImGuiCol color)
    {
        const auto vec = ImGui::GetStyleColorVec4(color);
        return {vec.x, vec.y, vec.z, vec.w};
    }

    inline Vec2 get_style_frame_padding()
    {
        return get_style().FramePadding;
    }

    inline float get_style_frame_border_size()
    {
        return get_style().FrameBorderSize;
    }

    inline Vec2 get_style_panel_padding()
    {
        return get_style().WindowPadding;
    }

    inline Vec2 get_style_item_spacing()
    {
        return get_style().ItemSpacing;
    }

    inline Vec2 get_style_item_inner_spacing()
    {
        return get_style().ItemInnerSpacing;
    }

    inline float get_style_alpha()
    {
        return get_style().Alpha;
    }

    inline ImGuiIO& get_io()
    {
        return ImGui::GetIO();
    }

    inline void push_style_var(ImGuiStyleVar style, const Vec2& pos)
    {
        ImGui::PushStyleVar(style, pos);
    }

    inline void push_style_var(ImGuiStyleVar style, float pos)
    {
        ImGui::PushStyleVar(style, pos);
    }

    inline void pop_style_var(int count = 1)
    {
        ImGui::PopStyleVar(count);
    }

    inline void open_popup(CStringView str_id, ImGuiPopupFlags popup_flags = 0)
    {
        return ImGui::OpenPopup(str_id.c_str(), popup_flags);
    }

    inline bool begin_popup(CStringView str_id, ImGuiWindowFlags flags = 0)
    {
        return ImGui::BeginPopup(str_id.c_str(), flags);
    }

    inline bool begin_popup_context_menu(CStringView str_id = nullptr, ImGuiPopupFlags popup_flags = 1)
    {
        return ImGui::BeginPopupContextItem(str_id.c_str(), popup_flags);
    }

    inline bool begin_popup_modal(CStringView name, bool* p_open = NULL, ImGuiWindowFlags flags = 0)
    {
        return ImGui::BeginPopupModal(name.c_str(), p_open, flags);
    }

    inline void end_popup()
    {
        ImGui::EndPopup();
    }

    inline Vec2 get_mouse_pos()
    {
        return ImGui::GetMousePos();
    }

    inline bool begin_menu_bar()
    {
        return ImGui::BeginMenuBar();
    }

    inline void end_menu_bar()
    {
        ImGui::EndMenuBar();
    }

    inline void set_mouse_cursor(ImGuiMouseCursor cursor_type)
    {
        ImGui::SetMouseCursor(cursor_type);
    }

    inline void set_next_item_width(float item_width)
    {
        ImGui::SetNextItemWidth(item_width);
    }

    inline void set_next_item_open(bool is_open)
    {
        ImGui::SetNextItemOpen(is_open);
    }

    inline void push_item_flag(ImGuiItemFlags option, bool enabled)
    {
        ImGui::PushItemFlag(option, enabled);
    }

    inline void pop_item_flag()
    {
        ImGui::PopItemFlag();
    }

    inline bool is_item_clicked(ImGuiMouseButton mouse_button = 0)
    {
        return ImGui::IsItemClicked(mouse_button);
    }

    inline bool is_item_hovered(ImGuiHoveredFlags flags = 0)
    {
        return ImGui::IsItemHovered(flags);
    }

    inline bool is_item_deactivated_after_edit()
    {
        return ImGui::IsItemDeactivatedAfterEdit();
    }

    inline Vec2 get_item_topleft()
    {
        return ImGui::GetItemRectMin();
    }

    inline Vec2 get_item_bottomright()
    {
        return ImGui::GetItemRectMax();
    }

    inline bool begin_table(CStringView str_id, int column, ImGuiTableFlags flags = 0, const Vec2& outer_size = {}, float inner_width = 0.0f)
    {
        return ImGui::BeginTable(str_id.c_str(), column, flags, outer_size, inner_width);
    }

    inline void table_setup_scroll_freeze(int cols, int rows)
    {
        ImGui::TableSetupScrollFreeze(cols, rows);
    }

    inline ImGuiTableSortSpecs* table_get_sort_specs()
    {
        return ImGui::TableGetSortSpecs();
    }

    inline void table_headers_row()
    {
        ImGui::TableHeadersRow();
    }

    inline bool table_set_column_index(int column_n)
    {
        return ImGui::TableSetColumnIndex(column_n);
    }

    inline void table_next_row(ImGuiTableRowFlags row_flags = 0, float min_row_height = 0.0f)
    {
        ImGui::TableNextRow(row_flags, min_row_height);
    }

    inline void table_setup_column(CStringView label, ImGuiTableColumnFlags flags = 0, float init_width_or_weight = 0.0f, ImGuiID user_id = 0)
    {
        ImGui::TableSetupColumn(label.c_str(), flags, init_width_or_weight, user_id);
    }

    inline void end_table()
    {
        ImGui::EndTable();
    }

    inline void push_style_color(ImGuiCol index, ImU32 col)
    {
        ImGui::PushStyleColor(index, col);
    }

    inline void push_style_color(ImGuiCol index, const Vec4& col)
    {
        ImGui::PushStyleColor(index, ImVec4{col});
    }

    inline void push_style_color(ImGuiCol index, const Color& c)
    {
        ImGui::PushStyleColor(index, {c.r, c.g, c.b, c.a});
    }

    inline void pop_style_color(int count = 1)
    {
        ImGui::PopStyleColor(count);
    }

    inline ImU32 get_color_ImU32(ImGuiCol index)
    {
        return ImGui::GetColorU32(index);
    }

    inline ImU32 to_ImU32(const Vec4& color)
    {
        return ImGui::ColorConvertFloat4ToU32(color);
    }

    inline float get_text_line_height()
    {
        return ImGui::GetTextLineHeight();
    }

    inline float get_text_line_height_with_spacing()
    {
        return ImGui::GetTextLineHeightWithSpacing();
    }

    inline float get_font_size()
    {
        return ImGui::GetFontSize();
    }

    inline Vec2 calc_text_size(CStringView text, bool hide_text_after_double_hash = false)
    {
        return ImGui::CalcTextSize(text.c_str(), text.c_str() + text.size(), hide_text_after_double_hash);
    }

    inline Vec2 get_panel_size()
    {
        return ImGui::GetWindowSize();
    }

    inline ImDrawList* get_panel_draw_list()
    {
        return ImGui::GetWindowDrawList();
    }

    inline ImDrawList* get_foreground_draw_list()
    {
        return ImGui::GetForegroundDrawList();
    }

    inline ImDrawListSharedData* get_draw_list_shared_data()
    {
        return ImGui::GetDrawListSharedData();
    }

    inline void show_demo_panel()
    {
        ImGui::ShowDemoWindow();
    }

    // a the "difference" added by a user-enacted manipulation with a `Gizmo`
    //
    // this can be left-multiplied by the original model matrix to apply the
    // user's transformation
    struct GizmoTransform final {
        Vec3 scale = {1.0f, 1.0f, 1.0f};
        EulerAngles rotation = {};
        Vec3 position = {};
    };

    // an operation that a ui `Gizmo` shall perform
    enum class GizmoOperation {
        Translate,
        Rotate,
        Scale,
        NUM_OPTIONS,
    };

    // the mode (coordinate space) that a `Gizmo` presents its manipulations in
    enum class GizmoMode {
        Local,
        World,
        NUM_OPTIONS,
    };

    // a UI gizmo that manipulates the given model matrix using user-interactable drag handles, arrows, etc.
    class Gizmo final {
    public:

        // if the user manipulated the gizmo, updates `model_matrix` to match the
        // post-manipulation transform and returns a world-space transform that
        // represents the "difference" added by the user's manipulation. I.e.:
        //
        //     transform_returned * model_matrix_before = model_matrix_after
        //
        // note: a user-enacted rotation/scale may not happen at the origin, so if
        //       you're thinking "oh I'm only handling rotation/scaling, so I'll
        //       ignore the translational part of the transform" then you're in
        //       for a nasty surprise: T(origin)*R*S*T(-origin)
        std::optional<GizmoTransform> draw(
            Mat4& model_matrix,  // edited in-place
            const Mat4& view_matrix,
            const Mat4& projection_matrix,
            const Rect& screenspace_rect
        );
        bool is_using() const;
        bool was_using() const { return was_using_last_frame_; }
        bool is_over() const;
        GizmoOperation operation() const { return operation_; }
        void set_operation(GizmoOperation op) { operation_ = op; }
        GizmoMode mode() const { return mode_; }
        void set_mode(GizmoMode mode) { mode_ = mode; }

        // updates the gizmo based on keyboard inputs (e.g. pressing `G` enables grab mode)
        bool handle_keyboard_inputs();
    private:
        UID id_;
        GizmoOperation operation_ = GizmoOperation::Translate;
        GizmoMode mode_ = GizmoMode::World;
        bool was_using_last_frame_ = false;
    };

    bool draw_gizmo_mode_selector(
        GizmoMode&
    );
    bool draw_gizmo_op_selector(
        GizmoOperation&,
        bool can_translate = true,
        bool can_rotate = true,
        bool can_scale = true
    );
}
