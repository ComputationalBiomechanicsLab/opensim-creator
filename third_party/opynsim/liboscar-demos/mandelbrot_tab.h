#pragma once

#include <liboscar/ui/tabs/tab.h>

namespace osc
{
    class Widget;

    class MandelbrotTab final : public Tab {
    public:
        static CStringView id();

        explicit MandelbrotTab(Widget*);

    private:
        bool impl_on_event(Event&) override;
        void impl_on_draw() override;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
