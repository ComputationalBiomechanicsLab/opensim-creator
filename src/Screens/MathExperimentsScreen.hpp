#pragma once

#include "src/Platform/Screen.hpp"

#include <SDL_events.h>

namespace osc
{
    // basic screen for personal math experiments
    class MathExperimentsScreen final : public Screen {
    public:
        MathExperimentsScreen();
        MathExperimentsScreen(MathExperimentsScreen const&) = delete;
        MathExperimentsScreen(MathExperimentsScreen&&) noexcept;
        MathExperimentsScreen& operator=(MathExperimentsScreen const&) = delete;
        MathExperimentsScreen& operator=(MathExperimentsScreen&&) noexcept;
        ~MathExperimentsScreen() noexcept override;

        void onMount() override;
        void onUnmount() override;
        void onEvent(SDL_Event const&) override;
        void tick() override;
        void draw() override;

    private:
        class Impl;
        Impl* m_Impl;
    };
}
