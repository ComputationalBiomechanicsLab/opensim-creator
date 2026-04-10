#pragma once

#include <liboscar/ui/tabs/tab.h>

#include <exception>

namespace osc { class Widget; }

namespace osc
{
    class ErrorTab final : public Tab {
    public:
        explicit ErrorTab(Widget&, const std::exception&);

    private:
        void impl_on_draw() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
