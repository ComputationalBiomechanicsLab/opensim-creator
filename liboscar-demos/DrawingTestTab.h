#pragma once

#include <liboscar/UI/Tabs/Tab.h>
#include <liboscar/Utils/CStringView.h>

namespace osc
{
    class DrawingTestTab : public Tab {
    public:
        static CStringView id();

        explicit DrawingTestTab(Widget*);

    private:
        bool impl_on_event(Event&) final;
        void impl_on_tick() final;
        void impl_on_draw() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
