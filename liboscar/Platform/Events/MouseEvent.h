#pragma once

#include <liboscar/Maths/Vec2.h>
#include <liboscar/Platform/Events/Event.h>
#include <liboscar/Platform/MouseButton.h>
#include <liboscar/Platform/MouseInputSource.h>

namespace osc
{
    // Represents an event related to a button press or motion of a mouse.
    //
    // Related: `MouseWheelEvent`.
    class MouseEvent final : public Event {
    public:
        static MouseEvent button_down(MouseInputSource source, MouseButton button)
        {
            return MouseEvent{EventType::MouseButtonDown, source, button};
        }

        static MouseEvent button_up(MouseInputSource source, MouseButton button)
        {
            return MouseEvent{EventType::MouseButtonUp, source, button};
        }

        static MouseEvent motion(MouseInputSource source, Vec2 relative_delta, Vec2 position_in_window)
        {
            return MouseEvent{source, relative_delta, position_in_window};
        }

        MouseInputSource input_source() const { return input_source_; }
        MouseButton button() const { return button_; }

        // Returns the relative delta vector of the mouse motion (i.e. how much the mouse
        // moved since the previous `MouseEvent`) in screen space and device-independent
        // pixels.
        Vec2 delta() const { return relative_delta_; }

        // Returns the location of the mouse cursor in screen space and device-independent
        // pixels.
        Vec2 location() const { return location_in_window_; }

    private:
        explicit MouseEvent(
            EventType event_type,
            MouseInputSource input_source,
            MouseButton button) :

            Event{event_type},
            input_source_{input_source},
            button_{button}
        {}

        explicit MouseEvent(
            MouseInputSource input_source,
            Vec2 relative_delta,
            Vec2 position_in_window) :

            Event{EventType::MouseMove},
            relative_delta_{relative_delta},
            location_in_window_{position_in_window},
            input_source_{input_source}
        {}

        Vec2 relative_delta_;
        Vec2 location_in_window_;
        MouseInputSource input_source_ = MouseInputSource::Mouse;
        MouseButton button_ = MouseButton::None;
    };
}
