#pragma once

#include <liboscar/ui/tabs/tab.h>

namespace osc
{
    class Widget;

    class ImPlotDemoTab final : public Tab {
    public:
        static CStringView id();

        explicit ImPlotDemoTab(Widget*);

    private:
        void impl_on_draw() override;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
