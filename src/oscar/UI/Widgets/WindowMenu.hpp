#pragma once

#include <memory>

namespace osc { class PanelManager; }

namespace osc
{
    class WindowMenu final {
    public:
        explicit WindowMenu(std::shared_ptr<PanelManager>);
        WindowMenu(WindowMenu const&) = delete;
        WindowMenu(WindowMenu&&) noexcept;
        WindowMenu& operator=(WindowMenu const&) = delete;
        WindowMenu& operator=(WindowMenu&&) noexcept;
        ~WindowMenu() noexcept;

        void onDraw();
    private:
        void drawContent();

        std::shared_ptr<PanelManager> m_PanelManager;
    };
}
