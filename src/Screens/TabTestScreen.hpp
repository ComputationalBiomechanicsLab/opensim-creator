#pragma once

#include "src/Platform/Screen.hpp"

#include <SDL_events.h>

namespace osc
{
    class TabTestScreen final : public Screen {
    public:
        TabTestScreen();
        TabTestScreen(TabTestScreen const&) = delete;
        TabTestScreen(TabTestScreen&&) noexcept;
        TabTestScreen& operator=(TabTestScreen const&) = delete;
        TabTestScreen& operator=(TabTestScreen&&) noexcept;
        ~TabTestScreen() noexcept override;

        void onMount() override;
        void onUnmount() override;
        void onEvent(SDL_Event const&) override;
        void tick(float) override;
        void draw() override;

    private:
        class Impl;
        Impl* m_Impl;
    };
}
