#pragma once

#include <memory>
#include <string_view>

namespace osc { class Simulation; }
namespace osc { class SimulatorUIAPI; }

namespace osc
{
    class SimulationDetailsPanel final {
    public:
        SimulationDetailsPanel(
            std::string_view panelName,
            SimulatorUIAPI*,
            std::shared_ptr<Simulation>
        );
        SimulationDetailsPanel(SimulationDetailsPanel const&) = delete;
        SimulationDetailsPanel(SimulationDetailsPanel&&) noexcept;
        SimulationDetailsPanel& operator=(SimulationDetailsPanel const&) = delete;
        SimulationDetailsPanel& operator=(SimulationDetailsPanel&&) noexcept;
        ~SimulationDetailsPanel() noexcept;

        void draw();

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}