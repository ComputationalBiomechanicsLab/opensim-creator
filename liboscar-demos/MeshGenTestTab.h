#pragma once

#include <liboscar/UI/Tabs/Tab.h>

namespace osc { class Widget; }

namespace osc
{
    class MeshGenTestTab final : public Tab {
    public:
        static CStringView id();

        explicit MeshGenTestTab(Widget*);

    private:
        void impl_on_draw() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
