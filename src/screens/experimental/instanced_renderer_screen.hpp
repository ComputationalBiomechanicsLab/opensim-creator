#pragma once

#include "src/screen.hpp"

#include <memory>

namespace osc {
    class Instanced_render_screen final : public Screen {
    public:
        struct Impl;
    private:
        std::unique_ptr<Impl> m_Impl;

    public:
        Instanced_render_screen();
        ~Instanced_render_screen() noexcept override;

        void on_mount() override;
        void on_unmount() override;
        void on_event(SDL_Event const&) override;
        void tick(float) override;
        void draw() override;
    };
}
