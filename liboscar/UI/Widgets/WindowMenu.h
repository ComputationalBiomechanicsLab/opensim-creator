#pragma once

#include <memory>

namespace osc { class PanelManager; }

namespace osc
{
    class WindowMenu final {
    public:
        explicit WindowMenu(std::shared_ptr<PanelManager>);
        WindowMenu(const WindowMenu&) = delete;
        WindowMenu(WindowMenu&&) noexcept;
        WindowMenu& operator=(const WindowMenu&) = delete;
        WindowMenu& operator=(WindowMenu&&) noexcept;
        ~WindowMenu() noexcept;

        void on_draw();
    private:
        void draw_content();

        std::shared_ptr<PanelManager> panel_manager_;
    };
}
