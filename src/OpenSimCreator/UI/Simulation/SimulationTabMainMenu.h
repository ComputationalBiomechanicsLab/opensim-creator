#pragma once

#include <OpenSimCreator/UI/MainUIScreen.h>

#include <memory>

namespace osc { class PanelManager; }
namespace osc { class Simulation; }

namespace osc
{
    class SimulationTabMainMenu final {
    public:
        SimulationTabMainMenu(
            MainUIScreen&,
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
