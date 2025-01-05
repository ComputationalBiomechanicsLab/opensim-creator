#include "QuitEvent.h"

#include <oscar/Platform/Events/Event.h>
#include <oscar/Platform/Events/EventType.h>
#include <oscar/Utils/Assertions.h>

#include <SDL3/SDL_events.h>

osc::QuitEvent::QuitEvent(const SDL_Event& e) :
    Event{EventType::Quit}
{
    OSC_ASSERT(e.type == SDL_EVENT_QUIT);
}
