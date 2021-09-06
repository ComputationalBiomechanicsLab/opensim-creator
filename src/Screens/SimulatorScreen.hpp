#pragma once

#include "src/Screen.hpp"

#include <SDL_events.h>

#include <memory>

namespace osc {
    struct MainEditorState;
}

namespace osc {
    class SimulatorScreen final : public Screen {
    public:
        struct Impl;
    private:
        std::unique_ptr<Impl> m_Impl;

    public:
        SimulatorScreen(std::shared_ptr<MainEditorState>);
        ~SimulatorScreen() noexcept override;

        void onMount() override;
        void onUnmount() override;
        void onEvent(SDL_Event const&) override;
        void tick(float) override;
        void draw() override;
    };
}
