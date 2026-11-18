#pragma once

#include <liboscar/ui/tabs/tab.h>

namespace osc
{
    class Widget;

    class CustomWidgetsTab final : public Tab {
    public:
        static CStringView id();

        explicit CustomWidgetsTab(Widget*);

    private:
        void impl_on_draw() override;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
