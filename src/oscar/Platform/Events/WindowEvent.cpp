#include "WindowEvent.h"

#include <oscar/Platform/Events/Event.h>
#include <oscar/Platform/Events/EventType.h>
#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/EnumHelpers.h>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_video.h>

osc::WindowEvent::WindowEvent(const SDL_Event& e) :
    Event{EventType::Window}
{
    static_assert(num_options<WindowEventType>() == 9);
    OSC_ASSERT(SDL_EVENT_WINDOW_FIRST <= e.type and e.type <= SDL_EVENT_WINDOW_LAST);

    switch (e.type) {
    case SDL_EVENT_WINDOW_MOUSE_ENTER:           type_ = WindowEventType::GainedMouseFocus;          break;
    case SDL_EVENT_WINDOW_MOUSE_LEAVE:           type_ = WindowEventType::LostMouseFocus;            break;
    case SDL_EVENT_WINDOW_FOCUS_GAINED:          type_ = WindowEventType::GainedKeyboardFocus;       break;
    case SDL_EVENT_WINDOW_FOCUS_LOST:            type_ = WindowEventType::LostKeyboardFocus;         break;
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:       type_ = WindowEventType::WindowClosed;              break;
    case SDL_EVENT_WINDOW_MOVED:                 type_ = WindowEventType::WindowMoved;               break;
    case SDL_EVENT_WINDOW_RESIZED:               type_ = WindowEventType::WindowResized;             break;
    case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED: type_ = WindowEventType::WindowDisplayScaleChanged; break;
    default:                                     type_ = WindowEventType::Unknown;                   break;
    }
    window_ = WindowID{SDL_GetWindowFromID(e.window.windowID)};
    window_id_ = e.window.windowID;
}
