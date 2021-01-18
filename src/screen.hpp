#pragma once

#include "application.hpp"

#include <cassert>

// screen: thin abstraction over an application screen
//
// this exists to separate top-level application concerns (e.g. event pumping,
// framerate throttling, exit handling, etc. etc.) from specific per-screen
// concerns (e.g. drawing stuff, handling screen-specific events, etc.)

namespace osmv {

    enum class Event_response {
        handled,
        ignored,
    };

    // basic state machine for a screen that may draw itself onto the
    // current window
    class Screen {
        Application* parent = nullptr;

    public:
        // called by the owning (parent) application just before it mounts the screen
        void on_application_mount(Application* a) {
            assert(a != nullptr);
            parent = a;
        }

        // application instance that owns this screen
        Application& application() const noexcept {
            assert(parent != nullptr);
            return *parent;
        }

        // request that the application transitions to a different screen
        //
        // The transition does not happen immediately. Application guarantees that it will happen
        // after a screen's `on_event`, `tick`, or `draw` function returns.
        template<typename T, typename... Args>
        void request_screen_transition(Args&&... args) {
            assert(parent != nullptr);
            parent->request_screen_transition<T>(std::forward<Args>(args)...);
        }

        // called by the application whenever an external event is received (e.g. mousemove)
        virtual Event_response on_event(SDL_Event const&) {
            return Event_response::ignored;
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
