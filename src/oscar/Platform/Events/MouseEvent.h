#pragma once

#include <oscar/Maths/Vec2.h>
#include <oscar/Platform/Events/Event.h>
#include <oscar/Platform/MouseButton.h>
#include <oscar/Platform/MouseInputSource.h>

union SDL_Event;

namespace osc
{
    class MouseEvent final : public Event {
    public:
        explicit MouseEvent(const SDL_Event&);

        MouseInputSource input_source() const { return input_source_; }
        MouseButton button() const { return button_; }

        // Returns the relative delta of the mouse motion (i.e. how much the mouse moved
        // since the previous `MouseEvent`) in device-independent pixels.
        Vec2 relative_delta() const { return relative_delta_; }

        // Returns the position of the mouse cursor in a top-left coordinate system in
        // virtual device-independent pixels.
        Vec2 position_in_window() const { return position_in_window_; }

    private:
        Vec2 relative_delta_;
        Vec2 position_in_window_;
        MouseInputSource input_source_ = MouseInputSource::Mouse;
        MouseButton button_ = MouseButton::None;
    };
}
