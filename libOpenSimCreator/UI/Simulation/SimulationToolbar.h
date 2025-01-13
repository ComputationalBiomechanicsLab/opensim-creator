#pragma once

#include <memory>
#include <string_view>

namespace osc { class Simulation; }
namespace osc { class ISimulatorUIAPI; }

namespace osc
{
    class SimulationToolbar final {
    public:
        SimulationToolbar(
            std::string_view label,
            ISimulatorUIAPI*,
            std::shared_ptr<Simulation>
        );
        SimulationToolbar(const SimulationToolbar&) = delete;
        SimulationToolbar(SimulationToolbar&&) noexcept;
        SimulationToolbar& operator=(const SimulationToolbar&) = delete;
        SimulationToolbar& operator=(SimulationToolbar&&) noexcept;
        ~SimulationToolbar() noexcept;

        void onDraw();

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
