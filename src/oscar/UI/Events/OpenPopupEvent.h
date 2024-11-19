#pragma once

#include <oscar/Platform/Event.h>
#include <oscar/UI/Popups/IPopup.h>

#include <memory>
#include <utility>

namespace osc
{
    class OpenPopupEvent final : public Event {
    public:
        explicit OpenPopupEvent(std::unique_ptr<IPopup> popup_to_open) :
            popup_to_open_{std::move(popup_to_open)}
        {
            enable_propagation();
        }

        bool has_tab() const { return popup_to_open_ != nullptr; }
        std::unique_ptr<IPopup> take_tab() { return std::move(popup_to_open_); }
    private:
        std::unique_ptr<IPopup> popup_to_open_;
    };
}
