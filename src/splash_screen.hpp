#pragma once

#include "screen.hpp"

#include <memory>

namespace osmv {
    struct Splash_screen_impl;

    class Splash_screen final : public Screen {
        std::unique_ptr<Splash_screen_impl> impl;

    public:
        Splash_screen();
        Splash_screen(Splash_screen const&) = delete;
        Splash_screen(Splash_screen&&) = delete;
        Splash_screen& operator=(Splash_screen const&) = delete;
        Splash_screen& operator=(Splash_screen&&) = delete;
        ~Splash_screen() noexcept override;

        Event_response on_event(SDL_Event const&) override;
        void draw() override;
    };
}
