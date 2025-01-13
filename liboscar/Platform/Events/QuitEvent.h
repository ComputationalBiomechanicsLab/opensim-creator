#pragma once

#include <liboscar/Platform/Events/Event.h>
#include <liboscar/Platform/Events/EventType.h>

namespace osc
{
    class QuitEvent final : public Event {
    public:
        explicit QuitEvent() : Event{EventType::Quit} {}
    };
}
