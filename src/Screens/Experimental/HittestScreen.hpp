#pragma once

#include "src/Screen.hpp"

#include <memory>

namespace osc
{
    // visual testing of hittesting implementation
    class HittestScreen final : public Screen {
    public:
        HittestScreen();
        ~HittestScreen() noexcept override;

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
