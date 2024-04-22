#pragma once

#include <memory>
#include <vector>

namespace osc { class IPopup; }

namespace osc
{
    // generic storage for a drawable popup stack
    class PopupManager final {
    public:
        PopupManager();
        PopupManager(const PopupManager&) = delete;
        PopupManager(PopupManager&&) noexcept;
        PopupManager& operator=(const PopupManager&) = delete;
        PopupManager& operator=(PopupManager&&) noexcept;
        ~PopupManager() noexcept;

        void push_back(std::shared_ptr<IPopup>);
        void on_mount() { openAll(); }
        void openAll();
        void onDraw();
        [[nodiscard]] bool empty();
        void clear();
    private:
        std::vector<std::shared_ptr<IPopup>> m_Popups;
    };
}
