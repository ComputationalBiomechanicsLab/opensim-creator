#pragma once

#include <liboscar/platform/widget.h>

namespace osc
{
    // META: this is a valid screen with `CookiecutterScreen` as a replaceable
    //       string that users can "Find+Replace" to make their own screen impl
    class CookiecutterScreen final : public Widget {
    public:
        explicit CookiecutterScreen();

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
