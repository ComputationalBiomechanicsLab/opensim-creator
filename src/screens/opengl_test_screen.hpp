#pragma once

#include "screen.hpp"

#include <SDL_events.h>

namespace osmv {
    struct Opengl_test_screen_impl;

    class Opengl_test_screen final : public Screen {
        Opengl_test_screen_impl* impl;
    public:
        Opengl_test_screen();
        Opengl_test_screen(Opengl_test_screen const&) = delete;
        Opengl_test_screen(Opengl_test_screen&&) = delete;
        Opengl_test_screen& operator=(Opengl_test_screen const&) = delete;
        Opengl_test_screen& operator=(Opengl_test_screen&&) = delete;
        ~Opengl_test_screen() noexcept override;

        bool on_event(SDL_Event const&) override;
        void draw() override;
    };
}
