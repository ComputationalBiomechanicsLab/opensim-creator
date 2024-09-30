#pragma once

#include <memory>

namespace osc { class PanelManager; }
namespace osc { class Simulation; }
namespace osc { class Widget; }

namespace osc
{
    class SimulationTabMainMenu final {
    public:
        SimulationTabMainMenu(
            Widget&,
            std::shared_ptr<Simulation>,
            std::shared_ptr<PanelManager>
        );
        SimulationTabMainMenu(const SimulationTabMainMenu&) = delete;
        SimulationTabMainMenu(SimulationTabMainMenu&&) noexcept;
        SimulationTabMainMenu& operator=(const SimulationTabMainMenu&) = delete;
        SimulationTabMainMenu& operator=(SimulationTabMainMenu&&) noexcept;
        ~SimulationTabMainMenu() noexcept;

        void onDraw();

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
