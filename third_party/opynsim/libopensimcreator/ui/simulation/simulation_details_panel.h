#pragma once

#include <liboscar/ui/panels/panel.h>

#include <memory>
#include <string_view>

namespace osc
{
    class Simulation;
    class SimulatorUIAPI;

    class SimulationDetailsPanel final : public Panel {
    public:
        explicit SimulationDetailsPanel(
            Widget* parent,
            std::string_view panel_name,
            SimulatorUIAPI*,
            std::shared_ptr<const Simulation>);

    private:
        void impl_draw_content() override;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
