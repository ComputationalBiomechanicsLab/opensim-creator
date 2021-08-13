#pragma once

#include "src/screen.hpp"

#include <memory>

namespace osc {
    class Tut4_hittesting_screen final : public Screen {
    public:
        struct Impl;
    private:
        std::unique_ptr<Impl> m_Impl;

    public:
        Tut4_hittesting_screen();
        ~Tut4_hittesting_screen() noexcept;

        void on_mount() override;
        void on_unmount() override;
        void on_event(SDL_Event const&) override;
        void tick(float) override;
        void draw() override;
    };
}
