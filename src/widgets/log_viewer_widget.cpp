#include "log_viewer_widget.hpp"

#include "src/log.hpp"

#include <imgui.h>

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

            ImGui::EndMenuBar();
        }

        auto& guarded_content = log::get_traceback_log();
        auto const& content = guarded_content.lock();
        for (log::Owned_log_msg const& msg : *content) {
            ImGui::Text("[%s] %s", log::to_c_str(msg.level), msg.payload.c_str());

            if (st.autoscroll) {
                ImGui::SetScrollHere();
            }
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();
}
