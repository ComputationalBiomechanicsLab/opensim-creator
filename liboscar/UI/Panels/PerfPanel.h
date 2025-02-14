#pragma once

#include <liboscar/UI/Panels/Panel.h>

#include <string_view>

namespace osc
{
    class PerfPanel final : public Panel {
    public:
        explicit PerfPanel(
            Widget* parent = nullptr,
            std::string_view panel_name = "Performance"
        );

    private:
        void impl_draw_content() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
