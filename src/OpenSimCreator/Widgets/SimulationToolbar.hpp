#pragma once

#include <memory>
#include <string_view>

namespace osc { class Simulation; }
namespace osc { class SimulatorUIAPI; }

namespace osc
{
    class SimulationToolbar final {
    public:
        SimulationToolbar(
            std::string_view,
            SimulatorUIAPI*,
            std::shared_ptr<Simulation>
        );
        SimulationToolbar(SimulationToolbar const&) = delete;
        SimulationToolbar(SimulationToolbar&&) noexcept;
        SimulationToolbar& operator=(SimulationToolbar const&) = delete;
        SimulationToolbar& operator=(SimulationToolbar&&) noexcept;
        ~SimulationToolbar() noexcept;

        void draw();

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
