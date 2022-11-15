#pragma once

#include <memory>

namespace osc
{
    class LogViewer final {
    public:
        LogViewer();
        LogViewer(LogViewer const&) = delete;
        LogViewer(LogViewer&&) noexcept = default;
        LogViewer& operator=(LogViewer const&) = delete;
        LogViewer& operator=(LogViewer&&) noexcept = default;
        ~LogViewer() noexcept;


        // assumes caller handles `ImGui::Begin(panel_name, nullptr, ImGuiWindowFlags_MenuBar)`
        void draw();

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
