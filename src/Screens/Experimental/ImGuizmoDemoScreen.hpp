#pragma once

#include "src/Screen.hpp"

#include <memory>

namespace osc
{
    // shows official ImGuizmo demo
    class ImGuizmoDemoScreen final : public Screen {
    public:
        ImGuizmoDemoScreen();
        ~ImGuizmoDemoScreen() noexcept override;

        void onMount() override;
        void onUnmount() override;
        void onEvent(SDL_Event const&) override;
        void draw() override;

        struct Impl;
    private:
        std::unique_ptr<Impl> m_Impl;
    };
}
