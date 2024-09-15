#pragma once

#include <SDL_events.h>

namespace osc
{
    class Event {
    public:
        explicit Event(SDL_Event& e) : inner_event_{&e} {}
        operator SDL_Event& () { return *inner_event_; }
        operator const SDL_Event& () const { return *inner_event_; }
    private:
        SDL_Event* inner_event_;
    };
}
