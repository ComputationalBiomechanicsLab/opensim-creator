#pragma once

#include <SDL_events.h>

namespace osc {

    // top-level "screen" abstraction
    //
    // the OSC GUI shows exactly one concrete instance of a `Screen`. This is
    // a low-level API for drawing things in the application. While showing a
    // `Screen`, the top-level application will:
    //
    // - pump GUI events and pump relevant ones through `on_event(e)` until all
    //   events are pumped
    // 
    // - call `tick(dt)` once
    //
    // - call `draw()` once
    //
    // - go back to event pumping
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
        virtual void tick(float dt) {
        }

        // called by the application whenever it wants the implementation to draw a frame into
        // the window's framebuffer
        virtual void draw() = 0;

        virtual ~Screen() noexcept = default;
    };
}
