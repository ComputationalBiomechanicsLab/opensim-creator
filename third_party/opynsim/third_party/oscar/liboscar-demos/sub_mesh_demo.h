#pragma once

#include <liboscar/platform/widget.h>

namespace osc
{
    class SubMeshDemo final : public Widget {
    public:
        static CStringView id();

        explicit SubMeshDemo(Widget*);

    private:
        void impl_on_draw() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
