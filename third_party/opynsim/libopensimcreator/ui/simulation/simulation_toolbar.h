#pragma once

#include <memory>
#include <string_view>

namespace osc
{
    class Simulation;
    class SimulatorUIAPI;

    class SimulationToolbar final {
    public:
        explicit SimulationToolbar(
            std::string_view label, SimulatorUIAPI*, std::shared_ptr<Simulation>);
        SimulationToolbar(const SimulationToolbar&) = delete;
        SimulationToolbar(SimulationToolbar&&) noexcept;
        SimulationToolbar& operator=(const SimulationToolbar&) = delete;
        SimulationToolbar& operator=(SimulationToolbar&&) noexcept;
        ~SimulationToolbar() noexcept;

        void on_draw();

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}
