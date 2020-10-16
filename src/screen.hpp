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

    struct Resp_Please_quit {};
    struct Resp_Transition_to { std::unique_ptr<Screen> new_screen; };
    struct Resp_Ok {};
    using Screen_response = std::variant<
        Resp_Please_quit,
        Resp_Transition_to,
        Resp_Ok
    >;

    // basic state machine for a screen that may draw itself onto the
    // current window
    struct Screen {
        virtual void init(Application&) = 0;
        virtual Screen_response handle_event(Application&, SDL_Event&) = 0;
        virtual void draw(Application&) = 0;
        virtual ~Screen() noexcept = default;
    };
}
