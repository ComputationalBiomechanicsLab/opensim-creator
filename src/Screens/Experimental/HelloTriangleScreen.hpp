#pragma once

#include "src/Screen.hpp"

#include <SDL_events.h>

#include <memory>

namespace osc {

    class HelloTriangleScreen final : public Screen {
    public:
        struct Impl;
    private:
        std::unique_ptr<Impl> m_Impl;

    public:
        HelloTriangleScreen();
        ~HelloTriangleScreen() noexcept override;

        void onEvent(SDL_Event const&) override;
        void tick(float) override;
        void draw() override;
    };
}
