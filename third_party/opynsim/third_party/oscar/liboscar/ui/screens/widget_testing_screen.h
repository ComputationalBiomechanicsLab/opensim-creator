#pragma once

#include <liboscar/platform/widget.h>

namespace osc
{
    class WidgetTestingScreen final : public Widget {
    public:
        explicit WidgetTestingScreen(std::unique_ptr<Widget>);

    private:
        void impl_on_mount() final;
        void impl_on_unmount() final;
        bool impl_on_event(Event&) final;
        void impl_on_tick() final;
        void impl_on_draw() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
