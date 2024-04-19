#pragma once

#include <oscar/Platform/IScreen.h>

#include <SDL_events.h>

#include <memory>

namespace osc { class TabRegistryEntry; }

namespace osc
{
    class TabTestingScreen final : public IScreen {
    public:
        explicit TabTestingScreen(TabRegistryEntry const&);
        TabTestingScreen(const TabTestingScreen&) = delete;
        TabTestingScreen(TabTestingScreen&&) noexcept = default;
        TabTestingScreen& operator=(const TabTestingScreen&) = delete;
        TabTestingScreen& operator=(TabTestingScreen&&) noexcept = default;
        ~TabTestingScreen() noexcept override = default;
    private:
        void impl_on_mount() override;
        void impl_on_unmount() override;
        void impl_on_event(const SDL_Event&) override;
        void impl_on_tick() override;
        void impl_on_draw() override;

        class Impl;
        std::shared_ptr<Impl> m_Impl;
    };
}
