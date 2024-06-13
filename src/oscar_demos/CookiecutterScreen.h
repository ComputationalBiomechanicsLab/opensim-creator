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
        CookiecutterScreen(const CookiecutterScreen&) = delete;
        CookiecutterScreen(CookiecutterScreen&&) noexcept;
        CookiecutterScreen& operator=(const CookiecutterScreen&) = delete;
        CookiecutterScreen& operator=(CookiecutterScreen&&) noexcept;
        ~CookiecutterScreen() noexcept override;

    private:
        void impl_on_mount() final;
        void impl_on_unmount() final;
        bool impl_on_event(const SDL_Event&) final;
        void impl_on_tick() final;
        void impl_on_draw() final;

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}
