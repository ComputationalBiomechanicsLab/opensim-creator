#pragma once

#include "src/Platform/Screen.hpp"

#include <SDL_events.h>

namespace osc
{
    // META: this is a valid screen with `CookiecutterScreen` as a replaceable
    //       string that users can "Find+Replace" to make their own screen impl
    class CookiecutterScreen final : public Screen {
    public:
        CookiecutterScreen();
        CookiecutterScreen(CookiecutterScreen const&) = delete;
        CookiecutterScreen(CookiecutterScreen&&) noexcept;
        CookiecutterScreen& operator=(CookiecutterScreen const&) = delete;
        CookiecutterScreen& operator=(CookiecutterScreen&&) noexcept;
        ~CookiecutterScreen() noexcept override;

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
