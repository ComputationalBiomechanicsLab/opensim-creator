#include "Event.h"

#include <oscar/Utils/Assertions.h>

#include <SDL_events.h>

#include <memory>

using namespace osc;

osc::DropFileEvent::DropFileEvent(const SDL_Event& e) :
    Event{e},
    path_{e.drop.file}
{}

std::unique_ptr<Event> osc::parse_into_event(const SDL_Event& e)
{
    if (e.type == SDL_DROPFILE and e.drop.file) {
        return std::make_unique<DropFileEvent>(e);
    }
    else {
        return std::make_unique<RawEvent>(e);
    }
}
