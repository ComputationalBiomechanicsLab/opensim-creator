#pragma once

#include <liboscar/Platform/Events/Event.h>
#include <liboscar/Platform/Events/EventType.h>

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
