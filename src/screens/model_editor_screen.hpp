#pragma once

#include "screen.hpp"

#include <SDL_events.h>

namespace osmv {
    struct Model_editor_screen_impl;

    class Model_editor_screen final : public Screen {
        Model_editor_screen_impl* impl;

    public:
        Model_editor_screen();
        Model_editor_screen(Model_editor_screen const&) = delete;
        Model_editor_screen(Model_editor_screen&&) = delete;
        Model_editor_screen& operator=(Model_editor_screen const&) = delete;
        Model_editor_screen& operator=(Model_editor_screen&&) = delete;
        ~Model_editor_screen() noexcept override;

        bool on_event(SDL_Event const&) override;
        void tick() override;
        void draw() override;
    };
}
