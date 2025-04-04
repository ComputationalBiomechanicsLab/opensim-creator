#pragma once

#include <liboscar/UI/Tabs/Tab.h>

#include <memory>

namespace osc { class Event; }
namespace osc { class Simulation; }
namespace osc { class Widget; }

namespace osc
{
    class SimulationTab final : public Tab {
    public:
        explicit SimulationTab(
            Widget*,
            std::shared_ptr<Simulation>
        );

    private:
        void impl_on_mount() final;
        void impl_on_unmount() final;
        bool impl_on_event(Event&) final;
        void impl_on_tick() final;
        void impl_on_draw_main_menu() final;
        void impl_on_draw() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
