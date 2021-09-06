#pragma once

#include <SDL_events.h>

namespace osc {
    // top-level screen shown by the application
    //
    // the application shows exactly one top-level `Screen` to the user at
    // any given time
    class Screen {
    public:
        virtual ~Screen() noexcept = default;

        // called before the app is about to start pump-/tick-/draw-ing the screen
        virtual void onMount() {}

        // called after the last time the app pumps/ticks/draws the screen
        virtual void onUnmount() {}

        // called by app to pump an event to the screen
        virtual void onEvent(SDL_Event const&) {}

        // called by app once per frame (float is a timedelta in seconds)
        virtual void tick(float) {}

        // called by app when the screen should render into the current framebuffer
        virtual void draw() = 0;
    };
}
