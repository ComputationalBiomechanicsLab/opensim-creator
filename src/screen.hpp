#pragma once

#include <cassert>

// screen: thin abstraction over an application screen
//
// this exists to separate top-level application concerns (e.g. event pumping,
// framerate throttling, exit handling, etc. etc.) from specific per-screen
// concerns (e.g. drawing stuff, handling screen-specific events, etc.)

union SDL_Event;

namespace osmv {
    class Application;

    // basic state machine for a screen that may draw itself onto the
    // current window
    class Screen {
        Application* parent;

    public:
        void on_application_mount(Application* a) {
            assert(a != nullptr);
            parent = a;
        }

        Application& application() const noexcept {
            return *parent;
        }

        // called by the application whenever an external event is received (e.g. mousemove)
        virtual void on_event(SDL_Event&) {
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
