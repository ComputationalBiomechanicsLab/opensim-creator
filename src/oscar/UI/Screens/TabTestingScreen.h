#pragma once

#include <oscar/Platform/Screen.h>

namespace osc { class TabRegistryEntry; }

namespace osc
{
    class TabTestingScreen final : public Screen {
    public:
        explicit TabTestingScreen(TabRegistryEntry const&);

    private:
        void impl_on_mount() final;
        void impl_on_unmount() final;
        bool impl_on_event(Event&) final;
        void impl_on_tick() final;
        void impl_on_draw() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
