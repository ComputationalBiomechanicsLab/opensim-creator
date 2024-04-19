#pragma once

#include <SDL_events.h>
#include <oscar/Platform/IScreen.h>

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
        void impl_on_mount() final;
        void impl_on_unmount() final;
        void impl_on_event(SDL_Event const&) final;
        void impl_on_tick() final;
        void impl_on_draw() final;

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
