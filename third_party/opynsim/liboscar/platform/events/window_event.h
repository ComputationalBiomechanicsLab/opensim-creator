#pragma once

#include <liboscar/platform/events/event.h>
#include <liboscar/platform/events/window_event_type.h>
#include <liboscar/platform/window_id.h>

#include <cstdint>

namespace osc
{
    // Represents a possible change in a single application window.
    class WindowEvent final : public Event {
    public:
        explicit WindowEvent(
            WindowEventType type,
            WindowID window_id) :

            type_{type},
            window_id_{window_id}
        {}

        WindowEventType type() const { return type_; }
        WindowID window_id() const { return window_id_; }
    private:
        WindowEventType type_ = WindowEventType::Unknown;
        WindowID window_id_;
    };
}
