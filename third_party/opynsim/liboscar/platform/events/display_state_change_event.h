#pragma once

#include <liboscar/platform/events/event.h>
#include <liboscar/platform/events/event_type.h>

namespace osc
{
    // Represents a display state change, such as:
    //
    // - display connected
    // - display disconnected
    // - display reoriented
    // - display resolution changed (maybe DPI change)
    class DisplayStateChangeEvent final : public Event {
    public:
        explicit DisplayStateChangeEvent() :
            Event{EventType::DisplayStateChange}
        {}
    };
}
