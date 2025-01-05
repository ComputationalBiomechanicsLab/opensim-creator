#include "DisplayStateChangeEvent.h"

#include <oscar/Platform/Events/Event.h>
#include <oscar/Platform/Events/EventType.h>

osc::DisplayStateChangeEvent::DisplayStateChangeEvent(const SDL_Event&) :
    Event{EventType::DisplayStateChange}
{}
