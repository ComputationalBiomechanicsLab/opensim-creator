#pragma once

namespace osc {
    class LogViewer final {
        bool autoscroll = true;

    public:
        // assumes caller handles `ImGui::Begin(panel_name, nullptr, ImGuiWindowFlags_MenuBar)`
        void draw();
    };
}
