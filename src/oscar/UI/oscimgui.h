#pragma once

#define IMGUI_USER_CONFIG <oscar/UI/oscimgui_config.h>

#include <imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>
#include <ImGuizmo.h>
#include <implot.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/UID.h>

#include <cstddef>
#include <utility>

namespace osc::ui
{
    inline void Text(CStringView sv)
    {
        ImGui::Text("%s", sv.c_str());
    }

    inline void Text(char const* fmt, ...) IM_FMTARGS(1)
    {
        va_list args;
        va_start(args, fmt);
        ImGui::Text(fmt, args);
        va_end(args);
    }

    inline void TextDisabled(CStringView sv)
    {
        ImGui::TextDisabled("%s", sv.c_str());
    }

    inline void TextDisabled(char const* fmt, ...) IM_FMTARGS(1)
    {
        va_list args;
        va_start(args, fmt);
        ImGui::TextDisabled(fmt, args);
        va_end(args);
    }

    inline void TextWrapped(CStringView sv)
    {
        ImGui::TextWrapped("%s", sv.c_str());
    }

    inline void TextWrapped(char const* fmt, ...) IM_FMTARGS(1)
    {
        va_list args;
        va_start(args, fmt);
        ImGui::TextWrapped(fmt, args);
        va_end(args);
    }

    inline void TextUnformatted(CStringView sv)
    {
        ImGui::TextUnformatted(sv.c_str());
    }

    inline bool BeginMenu(CStringView sv, bool enabled = true)
    {
        return ImGui::BeginMenu(sv.c_str(), enabled);
    }

    inline void EndMenu()
    {
        return ImGui::EndMenu();
    }

    template<typename... Args>
    inline bool MenuItem(Args&&... args)
    {
        return ImGui::MenuItem(std::forward<Args>(args)...);
    }

    inline void Columns(int count = 1, const char* id = nullptr, bool border = true)
    {
        ImGui::Columns(count, id, border);
    }

    inline void NextColumn()
    {
        ImGui::NextColumn();
    }

    inline void SameLine(float offset_from_start_x = 0.0f, float spacing = 1.0f)
    {
        ImGui::SameLine(offset_from_start_x, spacing);
    }

    inline bool IsMouseClicked(ImGuiMouseButton button, bool repeat = false)
    {
        return ImGui::IsMouseClicked(button, repeat);
    }

    inline bool Checkbox(const char* label, bool* v)
    {
        return ImGui::Checkbox(label, v);
    }

    inline bool InputFloat3(const char* label, float v[3], const char* format = "%.3f", ImGuiInputTextFlags flags = 0)
    {
        return ImGui::InputFloat3(label, v, format, flags);
    }

    inline bool Button(const char* label, ImVec2 const& size = ImVec2(0.0f, 0.0f))
    {
        return ImGui::Button(label, size);
    }

    inline void Dummy(ImVec2 const& size)
    {
        ImGui::Dummy(size);
    }

    inline bool Begin(const char* name, bool* p_open = nullptr, ImGuiWindowFlags flags = 0)
    {
        return ImGui::Begin(name, p_open, flags);
    }

    inline void End()
    {
        ImGui::End();
    }

    inline void BeginDisabled(bool disabled = true)
    {
        ImGui::BeginDisabled(disabled);
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

    inline void Separator()
    {
        ImGui::Separator();
    }

    inline bool IsKeyPressed(ImGuiKey key, bool repeat = true)
    {
        return ImGui::IsKeyPressed(key, repeat);
    }

    inline void PushStyleVar(ImGuiStyleVar style, ImVec2 const& pos)
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

    inline bool BeginPopup(const char* str_id, ImGuiWindowFlags flags = 0)
    {
        return ImGui::BeginPopup(str_id, flags);
    }

    inline bool BeginPopupContextItem(const char* str_id = nullptr, ImGuiPopupFlags popup_flags = 1)
    {
        return ImGui::BeginPopupContextItem(str_id, popup_flags);
    }

    inline bool BeginPopupModal(const char* name, bool* p_open = NULL, ImGuiWindowFlags flags = 0)
    {
        return ImGui::BeginPopupModal(name, p_open, flags);
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

    inline bool IsItemHovered(ImGuiHoveredFlags flags = 0)
    {
        return ImGui::IsItemHovered(flags);
    }

    inline bool IsItemDeactivatedAfterEdit()
    {
        return ImGui::IsItemDeactivatedAfterEdit();
    }
}
