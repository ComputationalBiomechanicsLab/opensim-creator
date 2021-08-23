#pragma once

#include "src/screen.hpp"

#include <memory>

namespace osc {
    class Component_3d_viewer_screen final : public Screen {
    public:
        struct Impl;
    private:
        std::unique_ptr<Impl> m_Impl;

    public:
        Component_3d_viewer_screen();
        ~Component_3d_viewer_screen() noexcept override;

        void on_mount() override;
        void on_unmount() override;
        void on_event(SDL_Event const&) override;
        void draw() override;
    };
}
