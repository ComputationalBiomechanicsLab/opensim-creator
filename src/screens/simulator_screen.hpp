#pragma once

#include "src/screens/screen.hpp"

#include <memory>

namespace osc {
    struct Main_editor_state;
}

namespace osc {
    class Simulator_screen final : public Screen {
    public:
        struct Impl;
    private:
        Impl* impl;

    public:
        Simulator_screen(std::shared_ptr<Main_editor_state>);
        Simulator_screen(Simulator_screen const&) = delete;
        Simulator_screen(Simulator_screen&&) = delete;
        Simulator_screen& operator=(Simulator_screen const&) = delete;
        Simulator_screen& operator=(Simulator_screen&&) = delete;
        ~Simulator_screen() noexcept;

        bool on_event(SDL_Event const&) override;
        void tick(float) override;
        void draw() override;
    };
}
