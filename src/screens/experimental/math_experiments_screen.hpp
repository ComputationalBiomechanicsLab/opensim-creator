#pragma once

#include "src/screen.hpp"

#include <SDL_events.h>

#include <memory>

namespace osc {

    class Math_experiments_screen final : public Screen {
    public:
        struct Impl;
    private:
        std::unique_ptr<Impl> m_Impl;

    public:
        Math_experiments_screen();
        ~Math_experiments_screen() noexcept override;

        void on_mount() override;
        void on_unmount() override;
        void on_event(SDL_Event const&) override;
        void tick(float) override;
        void draw() override;
    };
}
