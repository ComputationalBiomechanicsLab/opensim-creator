#pragma once

#define IMGUI_USER_CONFIG <oscar/UI/oscimgui_config.h>

#include <imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>
#include <ImGuizmo.h>
#include <implot.h>
#include <oscar/Utils/CStringView.h>

#include <utility>

namespace osc::ui
{
    inline void Text(CStringView sv)
    {
        ImGui::Text(sv.c_str());
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
}
