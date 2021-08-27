#pragma once

#include "src/screen.hpp"

#include <SDL_events.h>

#include <memory>

namespace osc {

    class Hellotriangle_screen final : public Screen {
    public:
        struct Impl;
    private:
        std::unique_ptr<Impl> impl;

    public:
        Hellotriangle_screen();
        ~Hellotriangle_screen() noexcept override;

        void on_event(SDL_Event const&) override;
        void tick(float) override;
        void draw() override;
    };
}
