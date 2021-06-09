#pragma once

#include "src/screens/screen.hpp"

#include <SDL_events.h>

namespace osc {

    // top-level splash screen
    //
    // this is shown when OSC boots and contains a list of previously opened files, etc.
    class Splash_screen final : public Screen {
        struct Impl;
        Impl* impl;

    public:
        Splash_screen();
        Splash_screen(Splash_screen const&) = delete;
        Splash_screen(Splash_screen&&) = delete;
        Splash_screen& operator=(Splash_screen const&) = delete;
        Splash_screen& operator=(Splash_screen&&) = delete;
        ~Splash_screen() noexcept override;

        void tick(float) override;
        void draw() override;
    };
}
