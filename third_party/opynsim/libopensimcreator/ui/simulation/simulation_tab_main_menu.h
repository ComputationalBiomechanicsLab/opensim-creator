#pragma once

#include <liboscar/platform/widget.h>

#include <memory>

namespace osc
{
    class PanelManager;
    class Simulation;

    class SimulationTabMainMenu final : public Widget {
    public:
        explicit SimulationTabMainMenu(
            Widget* parent, std::shared_ptr<Simulation>, std::shared_ptr<PanelManager>);

    private:
        void impl_on_draw() override;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
