#pragma once

#include <oscar/Platform/IScreen.h>

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
        void impl_on_mount() final;
        void impl_on_unmount() final;
        bool impl_on_event(const Event&) final;
        void impl_on_tick() final;
        void impl_on_draw() final;

        class Impl;
        std::shared_ptr<Impl> impl_;
    };
}
