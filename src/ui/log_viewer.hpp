#pragma once

namespace osc::ui::log_viewer {
    struct State final {
        bool autoscroll = true;
    };

    // assumes caller handles `ImGui::Begin(panel_name, nullptr, ImGuiWindowFlags_MenuBar)`
    void draw(State&);
}
