#pragma once

#include <memory>
#include <variant>

// screen: thin abstraction over an application screen
//
// this exists to separate top-level application concerns (e.g. event pumping,
// framerate throttling, exit handling, etc. etc.) from specific per-screen
// concerns (e.g. drawing stuff, handling screen-specific events, etc.)

union SDL_Event;

namespace osmv {
    class Application;
    struct Screen;

    struct Resp_quit {};
    struct Resp_transition { std::unique_ptr<Screen> new_screen; };
    struct Resp_ok {};
    using Screen_response = std::variant<Resp_quit, Resp_transition, Resp_ok>;

    // basic state machine for a screen that may draw itself onto the
    // current window
    struct Screen {

        // called by the application whenever an external event is received (e.g. mousemove)
        virtual Screen_response handle_event(Application&, SDL_Event&) {
            return Resp_ok{};
        }

        // called by the application each time a frame is about to be drawn: useful for handling
        // state changes that happen over time (e.g. animations, background processing)
        virtual Screen_response tick(Application&) {
            return Resp_ok{};
        }

        // called by the application whenever it wants the implementation to draw a frame into
        // the window's framebuffer
        virtual void draw(Application&) = 0;

        virtual ~Screen() noexcept = default;
    };
}
