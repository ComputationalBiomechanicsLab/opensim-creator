#pragma once

#include "src/screen.hpp"

#include <SDL_events.h>

#include <memory>

namespace osc {

    // META: this is a valid screen with `cookiecutter_screen` as a replaceable
    //       string that users can "Find+Replace" to make their own screen impl

    class cookiecutter_screen final : public Screen {
    public:
        struct Impl;
    private:
        std::unique_ptr<Impl> m_Impl;

    public:
        cookiecutter_screen();
        ~cookiecutter_screen() noexcept override;

        void on_mount() override;
        void on_unmount() override;
        void on_event(SDL_Event const&) override;
        void tick(float) override;
        void draw() override;
    };
}
