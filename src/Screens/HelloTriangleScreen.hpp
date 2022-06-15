#pragma once

#include "src/Platform/Screen.hpp"

#include <SDL_events.h>

namespace osc
{
    // basic test for graphics backend: can it display a triangle
    class HelloTriangleScreen final : public Screen {
    public:
        HelloTriangleScreen();
        HelloTriangleScreen(HelloTriangleScreen const&) = delete;
        HelloTriangleScreen(HelloTriangleScreen&&) noexcept;
        HelloTriangleScreen& operator=(HelloTriangleScreen const&) = delete;
        HelloTriangleScreen& operator=(HelloTriangleScreen&&) noexcept;
        ~HelloTriangleScreen() noexcept override;

        void onEvent(SDL_Event const&) override;
        void onTick() override;
        void onDraw() override;

    private:
        class Impl;
        Impl* m_Impl;
    };
}
