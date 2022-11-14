#pragma once

#include "src/OpenSimBindings/SimulationClock.hpp"
#include "src/OpenSimBindings/SimulationReport.hpp"

#include <SDL_events.h>

#include <memory>
#include <optional>

namespace osc { class Simulation; }

namespace osc
{
    class SimulationScrubber final {
    public:
        SimulationScrubber(std::shared_ptr<Simulation>);
        SimulationScrubber(SimulationScrubber const&) = delete;
        SimulationScrubber(SimulationScrubber&&) noexcept;
        SimulationScrubber& operator=(SimulationScrubber const&) = delete;
        SimulationScrubber& operator=(SimulationScrubber&&) noexcept;
        ~SimulationScrubber() noexcept;

        bool isPlayingBack() const;
        SimulationClock::time_point getScrubPositionInSimTime() const;
        std::optional<SimulationReport> tryLookupReportBasedOnScrubbing();
        void scrubTo(SimulationClock::time_point);
        void onTick();
        void onDraw();
        bool onEvent(SDL_Event const&);

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
