#pragma once

#include <liboscar/platform/events/event.h>
#include <liboscar/platform/events/event_type.h>

namespace osc
{
    // Represents a request for the application to shutdown (e.g. this may
    // be fired by the operating system when the user clicks a window's X
    // button or Alt+F4).
    class QuitEvent final : public Event {
    public:
        explicit QuitEvent() : Event{EventType::Quit} {}
    };
}
