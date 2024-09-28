#pragma once

#include <oscar/Platform/Screen.h>

namespace osc
{
    // META: this is a valid screen with `CookiecutterScreen` as a replaceable
    //       string that users can "Find+Replace" to make their own screen impl
    class CookiecutterScreen final : public Screen {
    public:
        explicit CookiecutterScreen(Widget&);

    private:
        void impl_on_mount() final;
        void impl_on_unmount() final;
        bool impl_on_event(Event&) final;
        void impl_on_tick() final;
        void impl_on_draw() final;

    private:
        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
