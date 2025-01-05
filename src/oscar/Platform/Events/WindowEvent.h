#pragma once

#include <oscar/Platform/Events/WindowEventType.h>
#include <oscar/Platform/Events/Event.h>
#include <oscar/Platform/WindowID.h>

#include <cstdint>

union SDL_Event;

namespace osc
{
    class WindowEvent final : public Event {
    public:
        explicit WindowEvent(const SDL_Event&);

        WindowEventType type() const { return type_; }
        WindowID window() const { return window_; }
        uint32_t window_id() const { return window_id_; }
    private:
        WindowEventType type_ = WindowEventType::Unknown;
        WindowID window_;
        uint32_t window_id_ = 0;
    };
}
