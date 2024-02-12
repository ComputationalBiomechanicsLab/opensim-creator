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
        TabTestingScreen(TabTestingScreen const&) = delete;
        TabTestingScreen(TabTestingScreen&&) noexcept = default;
        TabTestingScreen& operator=(TabTestingScreen const&) = delete;
        TabTestingScreen& operator=(TabTestingScreen&&) noexcept = default;
        ~TabTestingScreen() noexcept override = default;
    private:
        void implOnMount() override;
        void implOnUnmount() override;
        void implOnEvent(SDL_Event const&) override;
        void implOnTick() override;
        void implOnDraw() override;

        class Impl;
        std::shared_ptr<Impl> m_Impl;
    };
}
