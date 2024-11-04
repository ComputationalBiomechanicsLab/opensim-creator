#pragma once

#include <oscar/UI/Tabs/Tab.h>
#include <oscar/Utils/CStringView.h>

namespace osc { class Widget; }

namespace osc
{
    class RendererPerfTestingTab final : public Tab {
    public:
        static CStringView id();

        explicit RendererPerfTestingTab(Widget&);

    private:
        void impl_on_mount() final;
        void impl_on_unmount() final;
        void impl_on_tick() final;
        void impl_on_draw() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
