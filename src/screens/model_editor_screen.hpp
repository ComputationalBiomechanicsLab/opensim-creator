#pragma once

#include "src/screens/screen.hpp"

#include <SDL_events.h>

#include <memory>

namespace OpenSim {
    class Model;
}

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
        Impl* impl;

    public:
        // create the screen with a new model
        Model_editor_screen();

        // create the screen with an existing model
        Model_editor_screen(std::unique_ptr<OpenSim::Model>);

        // create the screen with existing top-level state
        //
        // this enables transitioning between top-level screens while
        // maintaining useful state between them (simulations, 
        // undo/redo, etc.)
        Model_editor_screen(std::shared_ptr<Main_editor_state>);

        Model_editor_screen(Model_editor_screen const&) = delete;
        Model_editor_screen(Model_editor_screen&&) = delete;
        Model_editor_screen& operator=(Model_editor_screen const&) = delete;
        Model_editor_screen& operator=(Model_editor_screen&&) = delete;
        ~Model_editor_screen() noexcept override;

        bool on_event(SDL_Event const&) override;
        void tick(float) override;
        void draw() override;
    };
}
