#pragma once

#include "src/screens/screen.hpp"

#include <SDL_events.h>

#include <memory>

namespace osc {
    struct Main_editor_state;
}

namespace osc {
    class Simulator_screen final : public Screen {
    public:
        struct Impl;
    private:
        std::unique_ptr<Impl> impl;

    public:
        Simulator_screen(std::shared_ptr<Main_editor_state>);
        ~Simulator_screen() noexcept override;

        bool on_event(SDL_Event const&) override;
        void tick(float) override;
        void draw() override;
    };
}
