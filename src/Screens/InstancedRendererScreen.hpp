#pragma once

#include "src/Platform/Screen.hpp"

#include <memory>

namespace osc
{
    // shows basic demo of instanced rendering
    class InstancedRendererScreen final : public Screen {
    public:
        InstancedRendererScreen();
        ~InstancedRendererScreen() noexcept override;

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
