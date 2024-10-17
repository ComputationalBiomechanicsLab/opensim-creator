#pragma once

#include <oscar/UI/Tabs/Tab.h>

namespace osc { class Widget; }

namespace osc
{
    class MeshHittestTab final : public Tab {
    public:
        static CStringView id();

        explicit MeshHittestTab(Widget&);

    private:
        void impl_on_tick() final;
        void impl_on_draw() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
