#pragma once

#include "src/Platform/Screen.hpp"

#include <SDL_events.h>

namespace osc
{
    // shows basic (not accelerated) mesh hittesting
    class MeshHittestScreen final : public Screen {
    public:
        MeshHittestScreen();
        MeshHittestScreen(MeshHittestScreen const&) = delete;
        MeshHittestScreen(MeshHittestScreen&&) noexcept;
        MeshHittestScreen& operator=(MeshHittestScreen const&) = delete;
        MeshHittestScreen& operator=(MeshHittestScreen&&) noexcept;
        ~MeshHittestScreen() noexcept override;

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
