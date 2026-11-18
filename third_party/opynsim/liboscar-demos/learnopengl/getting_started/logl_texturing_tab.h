#pragma once

#include <liboscar/ui/tabs/tab.h>

namespace osc
{
    class Widget;

    class LOGLTexturingTab final : public Tab {
    public:
        static CStringView id();

        explicit LOGLTexturingTab(Widget*);

    private:
        void impl_on_draw() override;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
