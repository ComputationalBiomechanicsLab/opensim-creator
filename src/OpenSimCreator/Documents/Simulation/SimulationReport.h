#pragma once

#include <OpenSimCreator/Documents/Simulation/SimulationClock.h>

#include <oscar/Utils/UID.h>

#include <optional>
#include <memory>
#include <unordered_map>

namespace SimTK { class State; }

namespace osc
{
    // reference-counted, immutable, simulation report
    class SimulationReport final {
    public:
        SimulationReport();
        explicit SimulationReport(SimTK::State&&);
        SimulationReport(SimTK::State&&, std::unordered_map<UID, float> auxiliaryValues);
        SimulationReport(const SimulationReport&);
        SimulationReport(SimulationReport&&) noexcept;
        SimulationReport& operator=(const SimulationReport&);
        SimulationReport& operator=(SimulationReport&&) noexcept;
        ~SimulationReport() noexcept;

        SimulationClock::time_point getTime() const;
        const SimTK::State& getState() const;
        SimTK::State& updStateHACK();  // necessary because of a bug in OpenSim PathWrap
        std::optional<float> getAuxiliaryValue(UID) const;

        friend bool operator==(const SimulationReport&, const SimulationReport&) = default;
    private:
        class Impl;
        std::shared_ptr<Impl> m_Impl;
    };
}
