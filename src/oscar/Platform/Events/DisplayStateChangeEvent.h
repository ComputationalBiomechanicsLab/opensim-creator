#pragma once

#include <oscar/Platform/Events/Event.h>
#include <oscar/Platform/Events/EventType.h>

namespace osc
{
    // fired off when the state of a display changes, such as:
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
