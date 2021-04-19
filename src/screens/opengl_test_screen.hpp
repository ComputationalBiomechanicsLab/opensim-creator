#pragma once

#include "src/screens/screen.hpp"

#include <SDL_events.h>

namespace osc {
    class Opengl_test_screen final : public Screen {
        struct Impl;
        Impl* impl;

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
