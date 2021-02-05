#pragma once

#include <SDL_events.h>

// screen: thin abstraction over an application screen
//
// this exists to separate top-level application concerns (e.g. event pumping,
// framerate throttling, exit handling, etc. etc.) from specific per-screen
// concerns (e.g. drawing stuff, handling screen-specific events, etc.)

namespace osmv {
    // basic state machine for a screen that may draw itself onto the
    // current window
    class Screen {
    public:
        // called by the application whenever an external event is received (e.g. mousemove)
        //
        // should return `true` if the event was handled, `false` otherwise
        virtual bool on_event(SDL_Event const&) {
            return false;
        }

        // called by the application each time a frame is about to be drawn: useful for handling
        // state changes that happen over time (e.g. animations, background processing)
        virtual void tick() {
        }

        // called by the application whenever it wants the implementation to draw a frame into
        // the window's framebuffer
        virtual void draw() = 0;

        virtual ~Screen() noexcept = default;
    };
}
