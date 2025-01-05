#include "TextInputEvent.h"

#include <oscar/Platform/Events/Event.h>
#include <oscar/Platform/Events/EventType.h>
#include <oscar/Utils/Assertions.h>

#include <SDL3/SDL_events.h>

osc::TextInputEvent::TextInputEvent(const SDL_Event& e) :
    Event{EventType::TextInput},
    utf8_text_{static_cast<const char*>(e.text.text)}
{
    OSC_ASSERT(e.type == SDL_EVENT_TEXT_INPUT);
}
