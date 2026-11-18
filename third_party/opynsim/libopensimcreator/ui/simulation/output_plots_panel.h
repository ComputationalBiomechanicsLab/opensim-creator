#pragma once

#include <liboscar/ui/panels/panel.h>

#include <memory>
#include <string_view>

namespace osc
{
    class Environment;
    class SimulatorUIAPI;

    class OutputPlotsPanel final : public Panel {
    public:
        explicit OutputPlotsPanel(
            Widget* parent,
            std::string_view panel_name,
            std::shared_ptr<Environment>,
            SimulatorUIAPI*);

    private:
        void impl_draw_content() override;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
