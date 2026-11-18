#pragma once

#include <liboscar/ui/tabs/tab.h>
#include <liboscar/utilities/c_string_view.h>

namespace osc
{
    class DrawingTestTab : public Tab {
    public:
        static CStringView id();

        explicit DrawingTestTab(Widget*);

    private:
        bool impl_on_event(Event&) override;
        void impl_on_tick() override;
        void impl_on_draw() override;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
