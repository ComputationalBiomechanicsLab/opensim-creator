#pragma once

#include "src/Platform/Screen.hpp"

#include <SDL_events.h>

namespace osc
{
    // shows official ImGuizmo demo
    class ImGuizmoDemoScreen final : public Screen {
    public:
        ImGuizmoDemoScreen();
        ImGuizmoDemoScreen(ImGuizmoDemoScreen const&) = delete;
        ImGuizmoDemoScreen(ImGuizmoDemoScreen&&) noexcept;
        ImGuizmoDemoScreen& operator=(ImGuizmoDemoScreen const&) = delete;
        ImGuizmoDemoScreen& operator=(ImGuizmoDemoScreen&&) noexcept;
        ~ImGuizmoDemoScreen() noexcept override;

        void onMount() override;
        void onUnmount() override;
        void onEvent(SDL_Event const&) override;
        void onDraw() override;

    private:
        class Impl;
        Impl* m_Impl;
    };
}
