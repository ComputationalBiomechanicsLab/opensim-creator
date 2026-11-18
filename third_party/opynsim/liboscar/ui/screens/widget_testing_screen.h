#pragma once

#include <liboscar/platform/widget.h>

namespace osc
{
    class WidgetTestingScreen final : public Widget {
    public:
        explicit WidgetTestingScreen(std::unique_ptr<Widget>);

    private:
        void impl_on_mount() override;
        void impl_on_unmount() override;
        bool impl_on_event(Event&) override;
        void impl_on_tick() override;
        void impl_on_draw() override;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
