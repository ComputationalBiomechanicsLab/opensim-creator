#pragma once

#include "src/Screen.hpp"

#include <memory>

namespace osc {
    class MeshHittestScreen final : public Screen {
    public:
        struct Impl;
    private:
        std::unique_ptr<Impl> m_Impl;

    public:
        MeshHittestScreen();
        ~MeshHittestScreen() noexcept override;

        void onMount() override;
        void onUnmount() override;
        void onEvent(SDL_Event const&) override;
        void tick(float) override;
        void draw() override;
    };
}
