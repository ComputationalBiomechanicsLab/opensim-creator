#pragma once

#include <liboscar/ui/tabs/tab.h>

namespace osc { class Widget; }

namespace osc
{
    class ImPlotDemoTab final : public Tab {
    public:
        static CStringView id();

        explicit ImPlotDemoTab(Widget*);

    private:
        void impl_on_draw() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
