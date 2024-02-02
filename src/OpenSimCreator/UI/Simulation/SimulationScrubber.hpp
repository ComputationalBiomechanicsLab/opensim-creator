#pragma once

#include <OpenSimCreator/Documents/Simulation/SimulationClock.hpp>

#include <memory>
#include <string_view>

namespace osc { class Simulation; }
namespace osc { class ISimulatorUIAPI; }

namespace osc
{
    class SimulationScrubber final {
    public:
        SimulationScrubber(
            std::string_view label,
            ISimulatorUIAPI*,
            std::shared_ptr<Simulation const>
        );
        SimulationScrubber(SimulationScrubber const&) = delete;
        SimulationScrubber(SimulationScrubber&&) noexcept;
        SimulationScrubber& operator=(SimulationScrubber const&) = delete;
        SimulationScrubber& operator=(SimulationScrubber&&) noexcept;
        ~SimulationScrubber() noexcept;

        void onDraw();

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
