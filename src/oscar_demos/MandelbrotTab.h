#pragma once

#include <oscar/UI/Tabs/Tab.h>

namespace osc { class Widget; }

namespace osc
{
    class MandelbrotTab final : public Tab {
    public:
        static CStringView id();

        explicit MandelbrotTab(Widget&);

    private:
        bool impl_on_event(Event&) final;
        void impl_on_draw() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
