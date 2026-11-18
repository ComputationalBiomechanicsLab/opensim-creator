#pragma once

#include <liboscar/ui/tabs/tab.h>

namespace osc
{
    class Screenshot;
    class Widget;

    class ScreenshotTab final : public Tab {
    public:
        explicit ScreenshotTab(Widget*, Screenshot&&);

    private:
        void impl_on_draw_main_menu() override;
        void impl_on_draw() override;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
