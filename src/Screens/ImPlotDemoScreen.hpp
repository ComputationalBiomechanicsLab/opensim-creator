#pragma once

#include "src/Platform/Screen.hpp"

#include <SDL_events.h>

namespace osc
{
    class ImPlotDemoScreen final : public Screen {
    public:
        ImPlotDemoScreen();
        ImPlotDemoScreen(ImPlotDemoScreen const&) = delete;
        ImPlotDemoScreen(ImPlotDemoScreen&&) noexcept;
        ImPlotDemoScreen& operator=(ImPlotDemoScreen const&) = delete;
        ImPlotDemoScreen& operator=(ImPlotDemoScreen&&) noexcept;
        ~ImPlotDemoScreen() noexcept override;

        void onMount() override;
        void onUnmount() override;
        void onEvent(SDL_Event const&) override;
        void tick(float) override;
        void draw() override;

        class Impl;
    private:
        Impl* m_Impl;
    };
}
