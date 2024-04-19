#pragma once

#define IMGUI_USER_CONFIG <oscar/UI/oscimgui_config.h>

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui/misc/cpp/imgui_stdlib.h>
#include <ImGuizmo.h>
#include <implot.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Maths/Rect.h>
#include <oscar/Maths/Vec.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/UID.h>

#include <cstddef>
#include <utility>

namespace osc::ui
{
    inline void AlignTextToFramePadding()
    {
        ImGui::AlignTextToFramePadding();
    }

    inline void Text(CStringView sv)
    {
        ImGui::TextUnformatted(sv.c_str(), sv.c_str() + sv.size());
    }

    inline void Text(const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        ImGui::TextV(fmt, args);
        va_end(args);
    }

    inline void TextDisabled(CStringView sv)
    {
        ImGui::TextDisabled("%s", sv.c_str());
    }

    inline void TextDisabled(const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        ImGui::TextDisabledV(fmt, args);
        va_end(args);
    }

    inline void TextWrapped(CStringView sv)
    {
        ImGui::TextWrapped("%s", sv.c_str());
    }

    inline void TextWrapped(const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        ImGui::TextWrappedV(fmt, args);
        va_end(args);
    }

    inline void TextUnformatted(CStringView sv)
    {
        ImGui::TextUnformatted(sv.c_str(), sv.c_str() + sv.size());
    }

    inline void Bullet()
    {
        ImGui::Bullet();
    }

    inline void BulletText(CStringView str)
    {
        ImGui::BulletText("%s", str.c_str());
    }

    inline bool TreeNodeEx(CStringView label, ImGuiTreeNodeFlags flags = 0)
    {
        return ImGui::TreeNodeEx(label.c_str(), flags);
    }

    inline float GetTreeNodeToLabelSpacing()
    {
        return ImGui::GetTreeNodeToLabelSpacing();
    }

    inline void TreePop()
    {
        ImGui::TreePop();
    }

    inline void ProgressBar(float fraction)
    {
        ImGui::ProgressBar(fraction);
    }

    inline bool BeginMenu(CStringView sv, bool enabled = true)
    {
        return ImGui::BeginMenu(sv.c_str(), enabled);
    }

    inline void EndMenu()
    {
        return ImGui::EndMenu();
    }

    inline bool MenuItem(CStringView label, CStringView shortcut = {}, bool selected = false, bool enabled = true)
    {
        return ImGui::MenuItem(label.c_str(), shortcut.empty() ? nullptr : shortcut.c_str(), selected, enabled);
    }

    inline bool MenuItem(CStringView label, CStringView shortcut, bool* p_selected, bool enabled = true)
    {
        return ImGui::MenuItem(label.c_str(), shortcut.empty() ? nullptr : shortcut.c_str(), p_selected, enabled);
    }

    inline bool BeginTabBar(CStringView str_id)
    {
        return ImGui::BeginTabBar(str_id.c_str());
    }

    inline void EndTabBar()
    {
        ImGui::EndTabBar();
    }

    inline bool BeginTabItem(CStringView label, bool* p_open = nullptr, ImGuiTabItemFlags flags = 0)
    {
        return ImGui::BeginTabItem(label.c_str(), p_open, flags);
    }

    inline void EndTabItem()
    {
        ImGui::EndTabItem();
    }

    inline bool TabItemButton(CStringView label)
    {
        return ImGui::TabItemButton(label.c_str());
    }

    inline void Columns(int count = 1, const char* id = nullptr, bool border = true)
    {
        ImGui::Columns(count, id, border);
    }

    inline float GetColumnWidth(int column_index = -1)
    {
        return ImGui::GetColumnWidth(column_index);
    }

    inline void NextColumn()
    {
        ImGui::NextColumn();
    }

    inline void SameLine(float offset_from_start_x = 0.0f, float spacing = -1.0f)
    {
        ImGui::SameLine(offset_from_start_x, spacing);
    }

    inline bool IsMouseClicked(ImGuiMouseButton button, bool repeat = false)
    {
        return ImGui::IsMouseClicked(button, repeat);
    }

    inline bool IsMouseClicked(ImGuiMouseButton button, ImGuiID owner_id, ImGuiInputFlags flags = 0)
    {
        return ImGui::IsMouseClicked(button, owner_id, flags);
    }

    inline bool IsMouseReleased(ImGuiMouseButton button)
    {
        return ImGui::IsMouseReleased(button);
    }

    inline bool IsMouseDown(ImGuiMouseButton button)
    {
        return ImGui::IsMouseDown(button);
    }

    inline bool IsMouseDragging(ImGuiMouseButton button, float lock_threshold = -1.0f)
    {
        return ImGui::IsMouseDragging(button, lock_threshold);
    }

    inline bool Selectable(CStringView label, bool* p_selected, ImGuiSelectableFlags flags = 0, const Vec2& size = {})
    {
        return ImGui::Selectable(label.c_str(), p_selected, flags, size);
    }

    inline bool Selectable(CStringView label, bool selected = false, ImGuiSelectableFlags flags = 0, const Vec2& size = {})
    {
        return ImGui::Selectable(label.c_str(), selected, flags, size);
    }

    inline bool Checkbox(CStringView label, bool* v)
    {
        return ImGui::Checkbox(label.c_str(), v);
    }

    inline bool CheckboxFlags(CStringView label, int* flags, int flags_value)
    {
        return ImGui::CheckboxFlags(label.c_str(), flags, flags_value);
    }

    inline bool CheckboxFlags(CStringView label, unsigned int* flags, unsigned int flags_value)
    {
        return ImGui::CheckboxFlags(label.c_str(), flags, flags_value);
    }

    inline bool SliderFloat(CStringView label, float* v, float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0)
    {
        return ImGui::SliderFloat(label.c_str(), v, v_min, v_max, format, flags);
    }

    inline bool InputScalar(CStringView label, ImGuiDataType data_type, void* p_data, const void* p_step = NULL, const void* p_step_fast = NULL, const char* format = NULL, ImGuiInputTextFlags flags = 0)
    {
        return ImGui::InputScalar(label.c_str(), data_type, p_data, p_step, p_step_fast, format, flags);
    }

    inline bool InputInt(CStringView label, int* v, int step = 1, int step_fast = 100, ImGuiInputTextFlags flags = 0)
    {
        return ImGui::InputInt(label.c_str(), v, step, step_fast, flags);
    }

    inline bool InputDouble(CStringView label, double* v, double step = 0.0, double step_fast = 0.0, const char* format = "%.6f", ImGuiInputTextFlags flags = 0)
    {
        return ImGui::InputDouble(label.c_str(), v, step, step_fast, format, flags);
    }

    inline bool InputFloat(CStringView label, float* v, float step = 0.0f, float step_fast = 0.0f, const char* format = "%.3f", ImGuiInputTextFlags flags = 0)
    {
        return ImGui::InputFloat(label.c_str(), v, step, step_fast, format, flags);
    }

    inline bool InputFloat3(CStringView label, float v[3], const char* format = "%.3f", ImGuiInputTextFlags flags = 0)
    {
        return ImGui::InputFloat3(label.c_str(), v, format, flags);
    }

    inline bool InputVec3(CStringView label, Vec3& v, const char* format = "%.3f", ImGuiInputTextFlags flags = 0)
    {
        return ImGui::InputFloat3(label.c_str(), &v.x, format, flags);
    }

    inline bool ColorEditRGB(CStringView label, Color& color)
    {
        return ImGui::ColorEdit3(label.c_str(), value_ptr(color));
    }

    inline bool ColorEditRGBA(CStringView label, Color& color)
    {
        return ImGui::ColorEdit4(label.c_str(), value_ptr(color));
    }

    inline bool Button(CStringView label, const Vec2& size = {})
    {
        return ImGui::Button(label.c_str(), size);
    }

    inline bool SmallButton(CStringView label)
    {
        return ImGui::SmallButton(label.c_str());
    }

    inline bool InvisibleButton(CStringView label, Vec2 size = {})
    {
        return ImGui::InvisibleButton(label.c_str(), size);
    }

    inline bool RadioButton(CStringView label, bool active)
    {
        return ImGui::RadioButton(label.c_str(), active);
    }

    inline bool CollapsingHeader(CStringView label)
    {
        return ImGui::CollapsingHeader(label.c_str());
    }

    inline void Dummy(const Vec2& size)
    {
        ImGui::Dummy(size);
    }

    inline bool BeginCombo(CStringView label, CStringView preview_value, ImGuiComboFlags flags = 0)
    {
        return ImGui::BeginCombo(label.c_str(), preview_value.empty() ? nullptr : preview_value.c_str(), flags);
    }

    inline void EndCombo()
    {
        ImGui::EndCombo();
    }

    inline bool Combo(CStringView label, int* current_item, const char* const items[], int items_count, int popup_max_height_in_items = -1)
    {
        return ImGui::Combo(label.c_str(), current_item, items, items_count, popup_max_height_in_items);
    }

    inline bool BeginListBox(CStringView label)
    {
        return ImGui::BeginListBox(label.c_str());
    }

    inline void EndListBox()
    {
        ImGui::EndListBox();
    }

    inline ImGuiViewport* GetMainViewport()
    {
        return ImGui::GetMainViewport();
    }

    inline ImGuiID DockSpaceOverViewport(const ImGuiViewport* viewport = NULL, ImGuiDockNodeFlags flags = 0, const ImGuiWindowClass* window_class = NULL)
    {
        return ImGui::DockSpaceOverViewport(viewport, flags, window_class);
    }

    inline bool Begin(CStringView name, bool* p_open = nullptr, ImGuiWindowFlags flags = 0)
    {
        return ImGui::Begin(name.c_str(), p_open, flags);
    }

    inline void End()
    {
        ImGui::End();
    }

    inline bool BeginChild(CStringView str_id, const Vec2& size = {}, ImGuiChildFlags child_flags = 0, ImGuiWindowFlags window_flags = 0)
    {
        return ImGui::BeginChild(str_id.c_str(), size, child_flags, window_flags);
    }

    inline void EndChild()
    {
        ImGui::EndChild();
    }

    inline void CloseCurrentPopup()
    {
        ImGui::CloseCurrentPopup();
    }

    inline void SetTooltip(const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        ImGui::SetTooltipV(fmt, args);
        va_end(args);
    }

    inline void SetScrollHereY()
    {
        ImGui::SetScrollHereY();
    }

    inline float GetFrameHeight()
    {
        return ImGui::GetFrameHeight();
    }

    inline Vec2 GetContentRegionAvail()
    {
        return ImGui::GetContentRegionAvail();
    }

    inline Vec2 GetCursorStartPos()
    {
        return ImGui::GetCursorStartPos();
    }

    inline Vec2 GetCursorPos()
    {
        return ImGui::GetCursorPos();
    }

    inline float GetCursorPosX()
    {
        return ImGui::GetCursorPosX();
    }

    inline void SetCursorPos(Vec2 p)
    {
        ImGui::SetCursorPos(p);
    }

    inline void SetCursorPosX(float local_x)
    {
        ImGui::SetCursorPosX(local_x);
    }

    inline Vec2 GetCursorScreenPos()
    {
        return ImGui::GetCursorScreenPos();
    }

    inline void SetCursorScreenPos(Vec2 pos)
    {
        ImGui::SetCursorScreenPos(pos);
    }

    inline void SetNextWindowPos(Vec2 pos, ImGuiCond cond = 0, Vec2 pivot = {})
    {
        ImGui::SetNextWindowPos(pos, cond, pivot);
    }

    inline void SetNextWindowSize(Vec2 size, ImGuiCond cond = 0)
    {
        ImGui::SetNextWindowSize(size, cond);
    }

    inline void SetNextWindowSizeConstraints(Vec2 size_min, Vec2 size_max)
    {
        ImGui::SetNextWindowSizeConstraints(size_min, size_max);
    }

    inline void SetNextWindowBgAlpha(float alpha)
    {
        ImGui::SetNextWindowBgAlpha(alpha);
    }

    inline bool IsWindowHovered(ImGuiHoveredFlags flags = 0)
    {
        return ImGui::IsWindowHovered(flags);
    }

    inline void BeginDisabled(bool disabled = true)
    {
        ImGui::BeginDisabled(disabled);
    }

    inline bool BeginTooltipNoWrap()
    {
        return ImGui::BeginTooltip();
    }

    inline void EndTooltipNoWrap()
    {
        ImGui::EndTooltip();
    }

    inline void EndDisabled()
    {
        ImGui::EndDisabled();
    }

    inline void PushID(UID id)
    {
        ImGui::PushID(static_cast<int>(id.get()));
    }

    inline void PushID(ptrdiff_t p)
    {
        ImGui::PushID(static_cast<int>(p));
    }

    inline void PushID(size_t i)
    {
        ImGui::PushID(static_cast<int>(i));
    }

    inline void PushID(int int_id)
    {
        ImGui::PushID(int_id);
    }

    inline void PushID(const void* ptr_id)
    {
        ImGui::PushID(ptr_id);
    }

    inline void PushID(CStringView str_id)
    {
        ImGui::PushID(str_id.data(), str_id.data() + str_id.size());
    }

    inline void PopID()
    {
        ImGui::PopID();
    }

    inline ImGuiID GetID(CStringView str_id)
    {
        return ImGui::GetID(str_id.c_str());
    }

    inline ImGuiItemFlags GetItemFlags()
    {
        return ImGui::GetItemFlags();
    }

    inline void ItemSize(Rect r)  // note: ImGui API assumes cursor is located at `p1` already
    {
        ImGui::ItemSize(ImRect{r.p1, r.p2});
    }

    inline bool ItemAdd(Rect bounds, ImGuiID id)
    {
        return ImGui::ItemAdd(ImRect{bounds.p1, bounds.p2}, id);
    }

    inline bool ItemHoverable(Rect bounds, ImGuiID id, ImGuiItemFlags item_flags)
    {
        return ImGui::ItemHoverable(ImRect{bounds.p1, bounds.p2}, id, item_flags);
    }

    inline void Separator()
    {
        ImGui::Separator();
    }

    inline void SeparatorEx(ImGuiSeparatorFlags flags)
    {
        ImGui::SeparatorEx(flags);
    }

    inline void NewLine()
    {
        ImGui::NewLine();
    }

    inline void Indent(float indent_w = 0.0f)
    {
        ImGui::Indent(indent_w);
    }

    inline void Unindent(float indent_w = 0.0f)
    {
        ImGui::Unindent(indent_w);
    }

    inline void SetKeyboardFocusHere()
    {
        ImGui::SetKeyboardFocusHere();
    }

    inline bool IsKeyPressed(ImGuiKey key, bool repeat = true)
    {
        return ImGui::IsKeyPressed(key, repeat);
    }

    inline bool IsKeyReleased(ImGuiKey key)
    {
        return ImGui::IsKeyReleased(key);
    }

    inline bool IsKeyDown(ImGuiKey key)
    {
        return ImGui::IsKeyDown(key);
    }

    inline ImGuiStyle& GetStyle()
    {
        return ImGui::GetStyle();
    }

    inline Color GetStyleColor(ImGuiCol color)
    {
        const auto vec = ImGui::GetStyleColorVec4(color);
        return {vec.x, vec.y, vec.z, vec.w};
    }

    inline Vec2 GetStyleFramePadding()
    {
        return GetStyle().FramePadding;
    }

    inline float GetStyleFrameBorderSize()
    {
        return GetStyle().FrameBorderSize;
    }

    inline Vec2 GetStyleWindowPadding()
    {
        return GetStyle().WindowPadding;
    }

    inline Vec2 GetStyleItemSpacing()
    {
        return GetStyle().ItemSpacing;
    }

    inline Vec2 GetStyleItemInnerSpacing()
    {
        return GetStyle().ItemInnerSpacing;
    }

    inline float GetStyleAlpha()
    {
        return GetStyle().Alpha;
    }

    inline ImGuiIO& GetIO()
    {
        return ImGui::GetIO();
    }

    inline void PushStyleVar(ImGuiStyleVar style, const Vec2& pos)
    {
        ImGui::PushStyleVar(style, pos);
    }

    inline void PushStyleVar(ImGuiStyleVar style, float pos)
    {
        ImGui::PushStyleVar(style, pos);
    }

    inline void PopStyleVar(int count = 1)
    {
        ImGui::PopStyleVar(count);
    }

    inline void OpenPopup(CStringView str_id, ImGuiPopupFlags popup_flags = 0)
    {
        return ImGui::OpenPopup(str_id.c_str(), popup_flags);
    }

    inline bool BeginPopup(CStringView str_id, ImGuiWindowFlags flags = 0)
    {
        return ImGui::BeginPopup(str_id.c_str(), flags);
    }

    inline bool BeginPopupContextItem(CStringView str_id = nullptr, ImGuiPopupFlags popup_flags = 1)
    {
        return ImGui::BeginPopupContextItem(str_id.c_str(), popup_flags);
    }

    inline bool BeginPopupModal(CStringView name, bool* p_open = NULL, ImGuiWindowFlags flags = 0)
    {
        return ImGui::BeginPopupModal(name.c_str(), p_open, flags);
    }

    inline void EndPopup()
    {
        ImGui::EndPopup();
    }

    inline Vec2 GetMousePos()
    {
        return ImGui::GetMousePos();
    }

    inline bool BeginMenuBar()
    {
        return ImGui::BeginMenuBar();
    }

    inline void EndMenuBar()
    {
        ImGui::EndMenuBar();
    }

    inline void SetMouseCursor(ImGuiMouseCursor cursor_type)
    {
        ImGui::SetMouseCursor(cursor_type);
    }

    inline void SetNextItemWidth(float item_width)
    {
        ImGui::SetNextItemWidth(item_width);
    }

    inline void SetNextItemOpen(bool is_open)
    {
        ImGui::SetNextItemOpen(is_open);
    }

    inline void PushItemFlag(ImGuiItemFlags option, bool enabled)
    {
        ImGui::PushItemFlag(option, enabled);
    }

    inline void PopItemFlag()
    {
        ImGui::PopItemFlag();
    }

    inline bool IsItemClicked(ImGuiMouseButton mouse_button = 0)
    {
        return ImGui::IsItemClicked(mouse_button);
    }

    inline bool IsItemHovered(ImGuiHoveredFlags flags = 0)
    {
        return ImGui::IsItemHovered(flags);
    }

    inline bool IsItemDeactivatedAfterEdit()
    {
        return ImGui::IsItemDeactivatedAfterEdit();
    }

    inline Vec2 GetItemRectMin()
    {
        return ImGui::GetItemRectMin();
    }

    inline Vec2 GetItemRectMax()
    {
        return ImGui::GetItemRectMax();
    }

    inline bool BeginTable(CStringView str_id, int column, ImGuiTableFlags flags = 0, const Vec2& outer_size = {}, float inner_width = 0.0f)
    {
        return ImGui::BeginTable(str_id.c_str(), column, flags, outer_size, inner_width);
    }

    inline void TableSetupScrollFreeze(int cols, int rows)
    {
        ImGui::TableSetupScrollFreeze(cols, rows);
    }

    inline ImGuiTableSortSpecs* TableGetSortSpecs()
    {
        return ImGui::TableGetSortSpecs();
    }

    inline void TableHeadersRow()
    {
        ImGui::TableHeadersRow();
    }

    inline bool TableSetColumnIndex(int column_n)
    {
        return ImGui::TableSetColumnIndex(column_n);
    }

    inline void TableNextRow(ImGuiTableRowFlags row_flags = 0, float min_row_height = 0.0f)
    {
        ImGui::TableNextRow(row_flags, min_row_height);
    }

    inline void TableSetupColumn(CStringView label, ImGuiTableColumnFlags flags = 0, float init_width_or_weight = 0.0f, ImGuiID user_id = 0)
    {
        ImGui::TableSetupColumn(label.c_str(), flags, init_width_or_weight, user_id);
    }

    inline void EndTable()
    {
        ImGui::EndTable();
    }

    inline void PushStyleColor(ImGuiCol index, ImU32 col)
    {
        ImGui::PushStyleColor(index, col);
    }

    inline void PushStyleColor(ImGuiCol index, const Vec4& col)
    {
        ImGui::PushStyleColor(index, ImVec4{col});
    }

    inline void PushStyleColor(ImGuiCol index, const Color& c)
    {
        ImGui::PushStyleColor(index, {c.r, c.g, c.b, c.a});
    }

    inline void PopStyleColor(int count = 1)
    {
        ImGui::PopStyleColor(count);
    }

    inline ImU32 GetColorU32(ImGuiCol index)
    {
        return ImGui::GetColorU32(index);
    }

    inline ImU32 ColorConvertFloat4ToU32(const Vec4& color)
    {
        return ImGui::ColorConvertFloat4ToU32(color);
    }

    inline float GetTextLineHeight()
    {
        return ImGui::GetTextLineHeight();
    }

    inline float GetTextLineHeightWithSpacing()
    {
        return ImGui::GetTextLineHeightWithSpacing();
    }

    inline float GetFontSize()
    {
        return ImGui::GetFontSize();
    }

    inline Vec2 CalcTextSize(CStringView text, bool hide_text_after_double_hash = false)
    {
        return ImGui::CalcTextSize(text.c_str(), text.c_str() + text.size(), hide_text_after_double_hash);
    }

    inline ImDrawList* GetWindowDrawList()
    {
        return ImGui::GetWindowDrawList();
    }

    inline ImDrawList* GetForegroundDrawList()
    {
        return ImGui::GetForegroundDrawList();
    }

    inline ImDrawListSharedData* GetDrawListSharedData()
    {
        return ImGui::GetDrawListSharedData();
    }

    inline void ShowDemoWindow()
    {
        ImGui::ShowDemoWindow();
    }
}
