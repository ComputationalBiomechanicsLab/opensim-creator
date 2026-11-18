#pragma once

#include <liboscar/ui/tabs/tab.h>
#include <liboscar/utilities/c_string_view.h>

namespace osc
{
    class Widget;

    class RendererPerfTestingTab final : public Tab {
    public:
        static CStringView id();

        explicit RendererPerfTestingTab(Widget*);

    private:
        void impl_on_mount() override;
        void impl_on_unmount() override;
        void impl_on_tick() override;
        void impl_on_draw() override;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
