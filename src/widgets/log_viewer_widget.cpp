#include "log_viewer_widget.hpp"

#include "src/log.hpp"

#include <imgui.h>

static ImVec4 color(osmv::log::level::Level_enum lvl) {
    using namespace osmv::log::level;

    switch (lvl) {
    case trace:
        return ImVec4{0.5f, 0.5f, 0.5f, 1.0f};
    case debug:
        return ImVec4{0.8f, 0.8f, 0.8f, 1.0f};
    case info:
        return ImVec4{0.5f, 0.5f, 1.0f, 1.0f};
    case warn:
        return ImVec4{1.0f, 1.0f, 0.0f, 1.0f};
    case err:
        return ImVec4{1.0f, 0.0f, 0.0f, 1.0f};
    case critical:
        return ImVec4{1.0f, 0.0f, 0.0f, 1.0f};
    default:
        return ImVec4{1.0f, 1.0f, 1.0f, 1.0f};
    }
}

void osmv::draw_log_viewer_widget(Log_viewer_widget_state& st, char const* panel_name) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0, 0.0));
    if (ImGui::Begin(panel_name, nullptr, ImGuiWindowFlags_MenuBar)) {
        if (ImGui::BeginMenuBar()) {
            // level selector
            {
                int lvl = static_cast<int>(log::get_traceback_level());
                if (ImGui::Combo("level", &lvl, log::level_cstring_names, log::level::NUM_LEVELS)) {
                    log::set_traceback_level(static_cast<log::level::Level_enum>(lvl));
                }
            }

            ImGui::SameLine();
            ImGui::Checkbox("autoscroll", &st.autoscroll);

            ImGui::SameLine();
            if (ImGui::Button("clear")) {
                log::get_traceback_log().lock()->clear();
            }

            ImGui::SameLine();
            if (ImGui::Button("turn off")) {
                log::set_traceback_level(log::level::off);
            }

            ImGui::Dummy(ImVec2{0.0f, 10.0f});

            ImGui::EndMenuBar();
        }

        auto& guarded_content = log::get_traceback_log();
        auto const& content = guarded_content.lock();
        for (log::Owned_log_msg const& msg : *content) {
            ImGui::PushStyleColor(ImGuiCol_Text, color(msg.level));
            ImGui::Text("[%s]", log::to_c_str(msg.level));
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::Text("%s", msg.payload.c_str());

            if (st.autoscroll) {
                ImGui::SetScrollHere();
            }
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();
}
