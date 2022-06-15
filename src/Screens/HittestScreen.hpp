#pragma once

#include "src/Platform/Screen.hpp"

#include <SDL_events.h>

namespace osc
{
    // visual testing of hittesting implementation
    class HittestScreen final : public Screen {
    public:
        HittestScreen();
        HittestScreen(HittestScreen const&) = delete;
        HittestScreen(HittestScreen&&) noexcept;
        HittestScreen& operator=(HittestScreen const&) = delete;
        HittestScreen& operator=(HittestScreen&&) noexcept;
        ~HittestScreen() noexcept override;

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
