#pragma once

#include <liboscar/ui/tabs/Tab.h>

namespace osc { class Widget; }

namespace osc
{
    class ImGuiDemoTab final : public Tab {
    public:
        static CStringView id();

        explicit ImGuiDemoTab(Widget*);

    private:
        void impl_on_draw() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
