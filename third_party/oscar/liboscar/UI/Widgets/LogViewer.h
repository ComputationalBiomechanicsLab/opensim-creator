#pragma once

#include <liboscar/Platform/Widget.h>

namespace osc
{
    class LogViewer final : public Widget {
    public:
        explicit LogViewer(Widget* parent);

    private:
        // assumes caller handles `ui::begin_panel(panel_name, nullptr, `ui::PanelFlag::MenuBar`)`
        void impl_on_draw() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
