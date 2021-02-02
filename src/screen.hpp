#pragma once

#include <SDL_events.h>

#include <memory>
#include <utility>

namespace osmv {
    class Application;
}

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
        friend class Application;
        friend struct Application_impl;

    public:
        // application instance that owns this screen
        Application& application() const noexcept;

        // request that the application transitions to a different screen
        //
        // The transition does not happen immediately. Application guarantees that it will happen
        // after a screen's `on_event`, `tick`, or `draw` function returns.
        template<typename T, typename... Args>
        void request_screen_transition(Args&&... args) {
            request_screen_transition(std::make_unique<T>(std::forward<Args>(args)...));
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

    private:
        // called by the owning (parent) application just before it mounts the screen
        void on_application_mount(Application* a);

        void request_screen_transition(std::unique_ptr<Screen>);
    };
}
