#pragma once

#include "src/Platform/Screen.hpp"

#include <SDL_events.h>

namespace osc
{
    // shows basic demo of instanced rendering
    class InstancedRendererScreen final : public Screen {
    public:
        InstancedRendererScreen();
        InstancedRendererScreen(InstancedRendererScreen const&) = delete;
        InstancedRendererScreen(InstancedRendererScreen&&) noexcept;
        InstancedRendererScreen& operator=(InstancedRendererScreen const&) = delete;
        InstancedRendererScreen& operator=(InstancedRendererScreen&&) noexcept;
        ~InstancedRendererScreen() noexcept override;

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
