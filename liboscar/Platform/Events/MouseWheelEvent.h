#pragma once

#include <liboscar/Maths/Vec2.h>
#include <liboscar/Platform/Events/Event.h>
#include <liboscar/Platform/Events/EventType.h>
#include <liboscar/Platform/MouseInputSource.h>

namespace osc
{
    // Represents a motion of a mouse wheel or similar (e.g. double-fingered scroll
    // on a touch screen) event.
    class MouseWheelEvent final : public Event {
    public:
        explicit MouseWheelEvent(Vec2 delta, MouseInputSource input_source) :
            Event{EventType::MouseWheel},
            delta_{delta},
            input_source_{input_source}
        {}

        MouseInputSource input_source() const { return input_source_; }

        // Returns the "delta" introduced by the wheel event. With typical
        // mouse wheels, this almost always translates to `{0.0f, -1.0f}`
        // when scrolling down and `{0.0f, +1.0f}` when scrolling up.
        Vec2 delta() const { return delta_; }
    private:
        Vec2 delta_;
        MouseInputSource input_source_ = MouseInputSource::TouchScreen;
    };
}
