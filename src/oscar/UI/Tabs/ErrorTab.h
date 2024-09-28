#pragma once

#include <oscar/UI/Tabs/Tab.h>

#include <exception>

namespace osc { class Widget; }

namespace osc
{
    class ErrorTab final : public Tab {
    public:
        ErrorTab(Widget&, const std::exception&);

    private:
        void impl_on_draw() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
