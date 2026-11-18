#pragma once

#include <memory>
#include <string_view>

namespace osc
{
    class Simulation;
    class SimulatorUIAPI;

    class SimulationScrubber final {
    public:
        explicit SimulationScrubber(
            std::string_view label, SimulatorUIAPI*, std::shared_ptr<const Simulation>);
        SimulationScrubber(const SimulationScrubber&) = delete;
        SimulationScrubber(SimulationScrubber&&) noexcept;
        SimulationScrubber& operator=(const SimulationScrubber&) = delete;
        SimulationScrubber& operator=(SimulationScrubber&&) noexcept;
        ~SimulationScrubber() noexcept;

        void on_draw();

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}
