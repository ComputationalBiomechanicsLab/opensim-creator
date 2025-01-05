#include "DropFileEvent.h"

#include <oscar/Platform/Events/Event.h>
#include <oscar/Platform/Events/EventType.h>
#include <oscar/Utils/Assertions.h>

#include <SDL3/SDL_events.h>

osc::DropFileEvent::DropFileEvent(const SDL_Event& e) :
    Event{EventType::DropFile},
    path_{e.drop.data}
{
    OSC_ASSERT(e.type == SDL_EVENT_DROP_FILE);
    OSC_ASSERT(e.drop.data != nullptr);
}
