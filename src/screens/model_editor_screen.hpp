#pragma once

#include "src/screen.hpp"

#include <SDL_events.h>

#include <memory>

namespace osc {
    struct Main_editor_state;
}

namespace osc {
    
    // main editor screen
    //
    // this is the top-level screen that users see when editing a model
    class Model_editor_screen final : public Screen {
    public:
        struct Impl;
    private:
        std::unique_ptr<Impl> impl;

    public:
        Model_editor_screen(std::shared_ptr<Main_editor_state>);

        Model_editor_screen(Model_editor_screen const&) = delete;
        Model_editor_screen(Model_editor_screen&&) noexcept;
        Model_editor_screen& operator=(Model_editor_screen const&) = delete;
        Model_editor_screen& operator=(Model_editor_screen&&) noexcept;
        ~Model_editor_screen() noexcept override;

        void on_mount() override;
        void on_unmount() override;
        void on_event(SDL_Event const&) override;
        void tick(float) override;
        void draw() override;
    };
}
