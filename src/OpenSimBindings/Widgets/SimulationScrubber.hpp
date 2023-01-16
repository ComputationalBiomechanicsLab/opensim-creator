#pragma once

#include "src/OpenSimBindings/SimulationClock.hpp"
#include "src/OpenSimBindings/SimulationReport.hpp"

#include <SDL_events.h>

#include <memory>
#include <optional>
#include <string_view>

namespace osc { class Simulation; }
namespace osc { class SimulatorUIAPI; }

namespace osc
{
    class SimulationScrubber final {
    public:
        SimulationScrubber(std::string_view, SimulatorUIAPI*, std::shared_ptr<Simulation>);
        SimulationScrubber(SimulationScrubber const&) = delete;
        SimulationScrubber(SimulationScrubber&&) noexcept;
        SimulationScrubber& operator=(SimulationScrubber const&) = delete;
        SimulationScrubber& operator=(SimulationScrubber&&) noexcept;
        ~SimulationScrubber() noexcept;

        void draw();

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
