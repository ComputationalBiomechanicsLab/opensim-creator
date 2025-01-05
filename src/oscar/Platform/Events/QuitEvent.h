#pragma once

#include <oscar/Platform/Events/Event.h>

union SDL_Event;

namespace osc
{
    class QuitEvent final : public Event {
    public:
        explicit QuitEvent(const SDL_Event&);
    };
}
