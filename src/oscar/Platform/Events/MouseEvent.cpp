#include "MouseEvent.h"

#include <oscar/Platform/Events/Event.h>
#include <oscar/Platform/Events/EventType.h>
#include <oscar/Platform/App.h>
#include <oscar/Platform/MouseButton.h>
#include <oscar/Utils/Conversion.h>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_touch.h>

using namespace osc;

template<>
struct osc::Converter<Uint8, osc::MouseButton> final {
    osc::MouseButton operator()(Uint8 sdlval) const
    {
        switch (sdlval) {
        case SDL_BUTTON_LEFT:   return MouseButton::Left;
        case SDL_BUTTON_RIGHT:  return MouseButton::Right;
        case SDL_BUTTON_MIDDLE: return MouseButton::Middle;
        case SDL_BUTTON_X1:     return MouseButton::Back;
        case SDL_BUTTON_X2:     return MouseButton::Forward;
        default:                return MouseButton::None;
        }
    }
};

osc::MouseEvent::MouseEvent(const SDL_Event& e) :
    Event{e.type == SDL_EVENT_MOUSE_BUTTON_DOWN ? EventType::MouseButtonDown : (e.type == SDL_EVENT_MOUSE_BUTTON_UP ? EventType::MouseButtonUp : EventType::MouseMove)}
{
    switch (e.type) {
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP: {
        input_source_ = e.button.which == SDL_TOUCH_MOUSEID ? MouseInputSource::TouchScreen : MouseInputSource::Mouse;
        button_ = to<MouseButton>(e.button.button);
        break;
    }
    case SDL_EVENT_MOUSE_MOTION: {
        // scales from SDL3 (events) to device-independent pixels
        const float sdl3_to_device_independent_ratio = App::get().os_to_main_window_device_independent_ratio();

        relative_delta_ = {static_cast<float>(e.motion.xrel), static_cast<float>(e.motion.yrel)};
        relative_delta_ *= sdl3_to_device_independent_ratio;
        position_in_window_ = {static_cast<float>(e.motion.x), static_cast<float>(e.motion.y)};
        position_in_window_ *= sdl3_to_device_independent_ratio;
        input_source_ = e.motion.which == SDL_TOUCH_MOUSEID ? MouseInputSource::TouchScreen : MouseInputSource::Mouse;
        break;
    }
    default: {
        throw std::runtime_error{"unknown mouse event type passed into an osc::MouseEvent"};
    }}
}
