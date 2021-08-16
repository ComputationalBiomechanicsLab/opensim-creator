#pragma once

#include "src/screen.hpp"

#include <memory>

namespace osc {

    class Opensim_modelstate_decoration_generator_screen final : public Screen {
    public:
        struct Impl;
    private:
        std::unique_ptr<Impl> m_Impl;

    public:
        Opensim_modelstate_decoration_generator_screen();
        ~Opensim_modelstate_decoration_generator_screen() noexcept override;

        void on_mount() override;
        void on_unmount() override;
        void on_event(SDL_Event const&) override;
        void draw() override;
    };
}
