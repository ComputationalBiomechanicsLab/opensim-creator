#pragma once

#include <liboscar/maths/vector2.h>
#include <liboscar/platform/events/event.h>
#include <liboscar/platform/mouse_button.h>
#include <liboscar/platform/mouse_input_source.h>

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

        static MouseEvent motion(MouseInputSource source, Vector2 relative_delta, Vector2 position_in_window)
        {
            return MouseEvent{source, relative_delta, position_in_window};
        }

        MouseInputSource input_source() const { return input_source_; }
        MouseButton button() const { return button_; }

        // Returns the relative delta vector of the mouse motion (i.e. how much the mouse
        // moved since the previous `MouseEvent`) in screen space and device-independent
        // pixels.
        Vector2 delta() const { return relative_delta_; }

        // Returns the position of the mouse cursor in screen space and device-independent
        // pixels.
        Vector2 position() const { return position_in_window_; }

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
            Vector2 relative_delta,
            Vector2 position_in_window) :

            Event{EventType::MouseMove},
            relative_delta_{relative_delta},
            position_in_window_{position_in_window},
            input_source_{input_source}
        {}

        Vector2 relative_delta_;
        Vector2 position_in_window_;
        MouseInputSource input_source_ = MouseInputSource::Mouse;
        MouseButton button_ = MouseButton::None;
    };
}
