#pragma once

#include <liboscar/ui/tabs/tab.h>

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
