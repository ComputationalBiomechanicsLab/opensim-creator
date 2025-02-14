#pragma once

#include <liboscar/Platform/Events/Event.h>
#include <liboscar/UI/Popups/Popup.h>

#include <memory>
#include <utility>

namespace osc
{
    class OpenPopupEvent final : public Event {
    public:
        explicit OpenPopupEvent(std::unique_ptr<Popup> popup_to_open) :
            popup_to_open_{std::move(popup_to_open)}
        {
            enable_propagation();
        }

        bool has_popup() const { return popup_to_open_ != nullptr; }
        std::unique_ptr<Popup> take_popup() { return std::move(popup_to_open_); }
    private:
        std::unique_ptr<Popup> popup_to_open_;
    };
}
