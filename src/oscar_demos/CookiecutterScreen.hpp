#pragma once

#include <oscar/Platform/IScreen.hpp>

#include <SDL_events.h>

#include <memory>

namespace osc
{
    // META: this is a valid screen with `CookiecutterScreen` as a replaceable
    //       string that users can "Find+Replace" to make their own screen impl
    class CookiecutterScreen final : public IScreen {
    public:
        CookiecutterScreen();
        CookiecutterScreen(CookiecutterScreen const&) = delete;
        CookiecutterScreen(CookiecutterScreen&&) noexcept;
        CookiecutterScreen& operator=(CookiecutterScreen const&) = delete;
        CookiecutterScreen& operator=(CookiecutterScreen&&) noexcept;
        ~CookiecutterScreen() noexcept override;

    private:
        void implOnMount() final;
        void implOnUnmount() final;
        void implOnEvent(SDL_Event const&) final;
        void implOnTick() final;
        void implOnDraw() final;

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
