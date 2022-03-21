#pragma once

#include "src/Platform/Screen.hpp"

#include <memory>

namespace osc
{
    // shows BVH-accelerated mesh hittesting
    class MeshHittestWithBVHScreen final : public Screen {
    public:
        MeshHittestWithBVHScreen();
        ~MeshHittestWithBVHScreen() noexcept override;

        void onMount() override;
        void onUnmount() override;
        void onEvent(SDL_Event const&) override;
        void tick(float) override;
        void draw() override;

        struct Impl;
    private:
        std::unique_ptr<Impl> m_Impl;
    };
}
