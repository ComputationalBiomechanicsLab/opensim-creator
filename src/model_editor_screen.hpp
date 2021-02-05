#pragma once

#include "screen.hpp"

#include <SDL_events.h>
#include <memory>

namespace osmv {
    struct Model_editor_screen_impl;
}

namespace osmv {
    class Model_editor_screen final : public Screen {
        std::unique_ptr<Model_editor_screen_impl> impl;
    public:
        Model_editor_screen(Application const&);
        Model_editor_screen(Model_editor_screen const&) = delete;
        Model_editor_screen(Model_editor_screen&&) = delete;
        Model_editor_screen& operator=(Model_editor_screen const&) = delete;
        Model_editor_screen& operator=(Model_editor_screen&&) = delete;
        ~Model_editor_screen() noexcept override;

        Event_response on_event(SDL_Event const&) override;
        void tick() override;
        void draw() override;
    };
}
