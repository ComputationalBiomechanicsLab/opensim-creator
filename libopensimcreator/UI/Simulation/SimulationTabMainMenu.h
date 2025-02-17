#pragma once

#include <liboscar/Platform/Widget.h>

#include <memory>

namespace osc { class PanelManager; }
namespace osc { class Simulation; }

namespace osc
{
    class SimulationTabMainMenu final : public Widget {
    public:
        explicit SimulationTabMainMenu(
            Widget* parent,
            std::shared_ptr<Simulation>,
            std::shared_ptr<PanelManager>
        );

    private:
        void impl_on_draw() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
