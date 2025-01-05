#pragma once

#include <oscar/Platform/Events/Event.h>
#include <oscar/Platform/Events/EventType.h>

namespace osc
{
    class QuitEvent final : public Event {
    public:
        explicit QuitEvent() : Event{EventType::Quit} {}
    };
}
