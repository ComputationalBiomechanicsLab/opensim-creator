#pragma once

#include "src/OpenSimBindings/SimulationClock.hpp"
#include "src/OpenSimBindings/SimulationReport.hpp"

#include <optional>

namespace osc { class VirtualSimulation; }
namespace osc { class OutputExtractor; }

namespace osc
{
    // API access to a simulator UI (e.g. simulator tab)
    //
    // this is how individual widgets within a simulator UI communicate with the simulator UI
    class SimulatorUIAPI {
    protected:
        SimulatorUIAPI() = default;
        SimulatorUIAPI(SimulatorUIAPI const&) = default;
        SimulatorUIAPI(SimulatorUIAPI&&) noexcept = default;
        SimulatorUIAPI& operator=(SimulatorUIAPI const&) = default;
        SimulatorUIAPI& operator=(SimulatorUIAPI&&) noexcept = default;
    public:
        virtual ~SimulatorUIAPI() noexcept = default;

        virtual VirtualSimulation& updSimulation() = 0;
        virtual SimulationClock::time_point getSimulationScrubTime() = 0;
        virtual void setSimulationScrubTime(SimulationClock::time_point) = 0;
        virtual std::optional<SimulationReport> trySelectReportBasedOnScrubbing() = 0;

        virtual int getNumUserOutputExtractors() const = 0;
        virtual OutputExtractor const& getUserOutputExtractor(int) const = 0;
        virtual void addUserOutputExtractor(OutputExtractor const&) = 0;
        virtual void removeUserOutputExtractor(int) = 0;
        virtual bool hasUserOutputExtractor(OutputExtractor const&) const = 0;
        virtual bool removeUserOutputExtractor(OutputExtractor const&) = 0;
    };
}
