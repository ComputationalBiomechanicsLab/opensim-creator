#pragma once

#include <liboscar/ui/panels/panel.h>

#include <string_view>

namespace osc { class Widget; }

namespace osc
{
    class LogViewerPanel final : public Panel {
    public:
        explicit LogViewerPanel(
            Widget* parent = nullptr,
            std::string_view panel_name = "Log"
        );

    private:
        void impl_draw_content() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
