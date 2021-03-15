#include "log_viewer_widget.hpp"

#include "src/utils/traceback_log.hpp"

#include <imgui.h>

void osmv::Log_viewer_widget::draw() {
    auto& guarded_content = osmv::get_traceback_log();
    auto const& content = guarded_content.lock();

    for (Owned_log_msg const& msg : *content) {
        ImGui::Text("[%s] %s", log::to_c_str(msg.level), msg.payload.c_str());
        ImGui::SetScrollHere();
    }
}
