#pragma once

#include <liboscar/platform/events/Event.h>
#include <liboscar/platform/events/EventType.h>

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
