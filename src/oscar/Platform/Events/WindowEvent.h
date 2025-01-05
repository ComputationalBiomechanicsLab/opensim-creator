#pragma once

#include <oscar/Platform/Events/WindowEventType.h>
#include <oscar/Platform/Events/Event.h>
#include <oscar/Platform/WindowID.h>

#include <cstdint>

namespace osc
{
    class WindowEvent final : public Event {
    public:
        explicit WindowEvent(
            WindowEventType type,
            WindowID window,
            uint32_t window_id) :

            type_{type},
            window_{window},
            window_id_{window_id}
        {}

        WindowEventType type() const { return type_; }
        WindowID window() const { return window_; }
        uint32_t window_id() const { return window_id_; }
    private:
        WindowEventType type_ = WindowEventType::Unknown;
        WindowID window_;
        uint32_t window_id_ = 0;
    };
}
