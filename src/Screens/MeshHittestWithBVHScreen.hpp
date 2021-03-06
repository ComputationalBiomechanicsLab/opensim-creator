#pragma once

#include "src/Platform/Screen.hpp"

#include <SDL_events.h>

namespace osc
{
    // shows BVH-accelerated mesh hittesting
    class MeshHittestWithBVHScreen final : public Screen {
    public:
        MeshHittestWithBVHScreen();
        MeshHittestWithBVHScreen(MeshHittestWithBVHScreen const&) = delete;
        MeshHittestWithBVHScreen(MeshHittestWithBVHScreen&&) noexcept;
        MeshHittestWithBVHScreen& operator=(MeshHittestWithBVHScreen const&) = delete;
        MeshHittestWithBVHScreen& operator=(MeshHittestWithBVHScreen&&) noexcept;
        ~MeshHittestWithBVHScreen() noexcept override;

        void onMount() override;
        void onUnmount() override;
        void onEvent(SDL_Event const&) override;
        void onTick() override;
        void onDraw() override;

    private:
        class Impl;
        Impl* m_Impl;
    };
}
