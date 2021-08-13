#pragma once

#include "src/screen.hpp"

#include <memory>

namespace osc {
    class Tut3_instanced_renderer_screen final : public Screen {
    public:
        struct Impl;
    private:
        std::unique_ptr<Impl> m_Impl;

    public:
        Tut3_instanced_renderer_screen();
        ~Tut3_instanced_renderer_screen() noexcept;

        void on_mount() override;
        void on_unmount() override;
        void on_event(SDL_Event const&) override;
        void tick(float) override;
        void draw() override;
    };
}
