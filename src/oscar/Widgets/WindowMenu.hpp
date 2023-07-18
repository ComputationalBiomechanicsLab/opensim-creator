#pragma once

#include <memory>

namespace osc { class PanelManager; }

namespace osc
{
    class WindowMenu final {
    public:
        WindowMenu(std::shared_ptr<PanelManager>);
        WindowMenu(WindowMenu const&) = delete;
        WindowMenu(WindowMenu&&) noexcept;
        WindowMenu& operator=(WindowMenu const&) = delete;
        WindowMenu& operator=(WindowMenu&&) noexcept;
        ~WindowMenu();

        void draw();
    private:
        void drawContent();

        std::shared_ptr<PanelManager> m_PanelManager;
    };
}
