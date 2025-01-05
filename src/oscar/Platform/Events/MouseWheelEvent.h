#pragma once

#include <oscar/Maths/Vec2.h>
#include <oscar/Platform/Events/Event.h>
#include <oscar/Platform/MouseInputSource.h>

union SDL_Event;

namespace osc
{
    class MouseWheelEvent final : public Event {
    public:
        explicit MouseWheelEvent(const SDL_Event&);

        MouseInputSource input_source() const { return input_source_; }
        Vec2 delta() const { return delta_; }
    private:
        Vec2 delta_;
        MouseInputSource input_source_ = MouseInputSource::TouchScreen;
    };
}
