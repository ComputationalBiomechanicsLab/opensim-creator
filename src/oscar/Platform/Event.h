#pragma once

#include <SDL_events.h>

namespace osc
{
    class Event {
    public:
        explicit Event(const SDL_Event& e) : inner_event_{&e} {}
        operator const SDL_Event& () const { return *inner_event_; }
    private:
        const SDL_Event* inner_event_;
    };
}
