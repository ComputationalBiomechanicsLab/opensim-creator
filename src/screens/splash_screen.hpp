#pragma once

#include "src/screens/screen.hpp"

#include <SDL_events.h>

namespace osmv {
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

        bool on_event(SDL_Event const&) override;
        void draw() override;
    };
}
