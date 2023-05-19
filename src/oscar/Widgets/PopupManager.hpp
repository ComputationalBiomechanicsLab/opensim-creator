#pragma once

#include <memory>
#include <vector>

namespace osc { class Popup; }

namespace osc
{
    // generic storage for a drawable popup stack
    class PopupManager final {
    public:
        PopupManager();
        PopupManager(PopupManager const&) = delete;
        PopupManager(PopupManager&&) noexcept;
        PopupManager& operator=(PopupManager const&) = delete;
        PopupManager& operator=(PopupManager&&) noexcept;
        ~PopupManager() noexcept;

        void push_back(std::shared_ptr<Popup>);
        void openAll();
        void draw();
    private:
        std::vector<std::shared_ptr<Popup>> m_Popups;
    };
}