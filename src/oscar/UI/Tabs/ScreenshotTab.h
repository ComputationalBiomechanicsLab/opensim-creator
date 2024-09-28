#pragma once

#include <oscar/UI/Tabs/Tab.h>

#include <memory>

namespace osc { class Screenshot; }
namespace osc { class Widget; }

namespace osc
{
    class ScreenshotTab final : public Tab {
    public:
        ScreenshotTab(Widget&, Screenshot&&);

    private:
        void impl_on_draw_main_menu() final;
        void impl_on_draw() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
