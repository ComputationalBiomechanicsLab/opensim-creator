#pragma once

#include "src/Platform/Screen.hpp"

#include <SDL_events.h>

namespace osc
{
    class RendererHelloTriangleScreen final : public Screen {
    public:
        RendererHelloTriangleScreen();
        RendererHelloTriangleScreen(RendererHelloTriangleScreen const&) = delete;
        RendererHelloTriangleScreen(RendererHelloTriangleScreen&&) noexcept;
        RendererHelloTriangleScreen& operator=(RendererHelloTriangleScreen const&) = delete;
        RendererHelloTriangleScreen& operator=(RendererHelloTriangleScreen&&) noexcept;
        ~RendererHelloTriangleScreen() noexcept override;

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
