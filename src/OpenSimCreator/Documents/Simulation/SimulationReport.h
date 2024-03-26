#pragma once

#include <OpenSimCreator/Documents/Simulation/AuxiliaryValue.h>
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
        SimulationReport(SimulationReport const&);
        SimulationReport(SimulationReport&&) noexcept;
        SimulationReport& operator=(SimulationReport const&);
        SimulationReport& operator=(SimulationReport&&) noexcept;
        ~SimulationReport() noexcept;

        SimulationClock::time_point getTime() const;
        SimTK::State&& stealState() &&;
        std::optional<float> getAuxiliaryValue(UID) const;
        std::vector<AuxiliaryValue> getAllAuxiliaryValuesHACK() const;

        friend bool operator==(SimulationReport const&, SimulationReport const&) = default;
    private:
        class Impl;
        std::shared_ptr<Impl> m_Impl;
    };
}
