#include "MouseWheelEvent.h"

#include <oscar/Platform/Events/Event.h>
#include <oscar/Platform/Events/EventType.h>
#include <oscar/Utils/Assertions.h>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_touch.h>

osc::MouseWheelEvent::MouseWheelEvent(const SDL_Event& e) :
    Event{EventType::MouseWheel},
    delta_{e.wheel.x, e.wheel.y},
    input_source_{e.wheel.which == SDL_TOUCH_MOUSEID ? MouseInputSource::TouchScreen : MouseInputSource::Mouse}
{
    OSC_ASSERT(e.type == SDL_EVENT_MOUSE_WHEEL);
}
