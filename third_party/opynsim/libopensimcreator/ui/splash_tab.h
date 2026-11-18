#pragma once

#include <liboscar/ui/tabs/tab.h>

namespace osc
{
    class Event;
    class Widget;

    class SplashTab final : public Tab {
    public:
        explicit SplashTab(Widget* parent);

    private:
        void impl_on_mount() override;
        void impl_on_unmount() override;
        bool impl_on_event(Event&) override;
        void impl_on_draw_main_menu() override;
        void impl_on_draw() override;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
