#pragma once

#include <oscar/UI/Panels/Panel.h>

#include <string_view>

namespace osc
{
    class LogViewerPanel final : public Panel {
    public:
        explicit LogViewerPanel(std::string_view panel_name = "Log");

    private:
        void impl_draw_content() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
