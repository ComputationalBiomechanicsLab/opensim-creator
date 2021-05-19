#pragma once

#include "src/screens/screen.hpp"

#include <memory>

namespace OpenSim {
    class Model;
}

namespace osc {

    // a screen that shows the model and enables the user to simulate the model
    // in a forward-dynamic simulation
    class Show_model_screen final : public Screen {
    public:
        struct Impl;
    private:
        Impl* impl;

    public:
        Show_model_screen(std::unique_ptr<OpenSim::Model>);
        Show_model_screen(Show_model_screen const&) = delete;
        Show_model_screen(Show_model_screen&&) = delete;
        Show_model_screen& operator=(Show_model_screen const&) = delete;
        Show_model_screen& operator=(Show_model_screen&&) = delete;
        ~Show_model_screen() noexcept override;

        bool on_event(SDL_Event const&) override;
        void tick() override;
        void draw() override;
    };
}
