#pragma once

#include "src/Platform/Screen.hpp"

#include <SDL_events.h>

namespace osc
{
    // screen that shows a UiModelViewer widget in isolation
    class UiModelViewerScreen final : public Screen {
    public:
        UiModelViewerScreen();
        UiModelViewerScreen(UiModelViewerScreen const&) = delete;
        UiModelViewerScreen(UiModelViewerScreen&&) noexcept;
        UiModelViewerScreen& operator=(UiModelViewerScreen const&) = delete;
        UiModelViewerScreen& operator=(UiModelViewerScreen&&) noexcept;
        ~UiModelViewerScreen() noexcept override;

        void onMount() override;
        void onUnmount() override;
        void onEvent(SDL_Event const&) override;
        void tick(float) override;
        void draw() override;

        class Impl;
    private:
        Impl* m_Impl;
    };
}
