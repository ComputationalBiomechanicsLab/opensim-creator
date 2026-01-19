#pragma once

#include <libopensimcreator/Documents/Simulation/SimulationClock.h>

#include <libopynsim/Documents/StateViewWithMetadata.h>
#include <liboscar/utils/uid.h>

#include <optional>
#include <memory>
#include <unordered_map>

namespace SimTK { class State; }

namespace osc
{
    // reference-counted, immutable, simulation report
    class SimulationReport final : public StateViewWithMetadata {
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
        SimTK::State& updStateHACK();  // necessary because of a bug in OpenSim PathWrap

        friend bool operator==(const SimulationReport&, const SimulationReport&) = default;
    private:
        const SimTK::State& implGetState() const final;
        std::optional<float> implGetAuxiliaryValue(UID) const final;

        class Impl;
        std::shared_ptr<Impl> m_Impl;
    };
}
