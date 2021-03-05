#pragma once

#include "src/screens/screen.hpp"

#include <SDL_events.h>

#include <memory>

namespace OpenSim {
    class Model;
}

namespace osmv {
    class Model_editor_screen final : public Screen {
        struct Impl;
        Impl* impl;

    public:
        Model_editor_screen();
        Model_editor_screen(std::unique_ptr<OpenSim::Model>);
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
