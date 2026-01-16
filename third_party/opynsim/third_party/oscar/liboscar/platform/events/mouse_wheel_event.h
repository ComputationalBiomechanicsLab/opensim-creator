#pragma once

#include <liboscar/maths/vector2.h>
#include <liboscar/platform/events/event.h>
#include <liboscar/platform/events/event_type.h>
#include <liboscar/platform/mouse_input_source.h>

namespace osc
{
    // Represents a motion of a mouse wheel or similar (e.g. double-fingered scroll
    // on a touch screen) event.
    class MouseWheelEvent final : public Event {
    public:
        explicit MouseWheelEvent(Vector2 delta, MouseInputSource input_source) :
            Event{EventType::MouseWheel},
            delta_{delta},
            input_source_{input_source}
        {}

        MouseInputSource input_source() const { return input_source_; }

        // Returns the "delta" introduced by the wheel event. With typical
        // mouse wheels, this almost always translates to `{0.0f, -1.0f}`
        // when scrolling down and `{0.0f, +1.0f}` when scrolling up.
        Vector2 delta() const { return delta_; }
    private:
        Vector2 delta_;
        MouseInputSource input_source_ = MouseInputSource::TouchScreen;
    };
}
