#pragma once

#include <oscar/UI/Panels/Panel.h>

#include <memory>
#include <string_view>

namespace osc { class Simulation; }
namespace osc { class ISimulatorUIAPI; }

namespace osc
{
    class SimulationDetailsPanel final : public Panel {
    public:
        explicit SimulationDetailsPanel(
            std::string_view panelName,
            ISimulatorUIAPI*,
            std::shared_ptr<const Simulation>
        );

    private:
        void impl_draw_content() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
