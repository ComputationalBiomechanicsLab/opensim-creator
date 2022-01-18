#pragma once

#include "src/Screen.hpp"

#include <SDL_events.h>

#include <memory>

namespace osc
{
    // basic test for graphics backend: can it display a triangle
    class HelloTriangleScreen final : public Screen {
    public:
        HelloTriangleScreen();
        ~HelloTriangleScreen() noexcept override;

        void onEvent(SDL_Event const&) override;
        void tick(float) override;
        void draw() override;

        struct Impl;
    private:
        std::unique_ptr<Impl> m_Impl;
    };
}
