#pragma once

#include <oscar/UI/Tabs/Tab.h>

namespace osc { class Widget; }

namespace osc
{
    class ImGuizmoDemoTab final : public Tab {
    public:
        static CStringView id();

        explicit ImGuizmoDemoTab(Widget&);

    private:
        void impl_on_draw() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
