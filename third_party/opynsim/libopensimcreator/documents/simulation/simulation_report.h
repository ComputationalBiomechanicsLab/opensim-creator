#pragma once

#include <libopensimcreator/documents/simulation/simulation_clock.h>

#include <libopynsim/documents/state_view_with_metadata.h>
#include <liboscar/utilities/uid.h>

#include <optional>
#include <memory>
#include <unordered_map>

namespace SimTK
{
    class State;
}

namespace osc
{
    // reference-counted, immutable, simulation report
    class SimulationReport final : public opyn::StateViewWithMetadata {
    public:
        SimulationReport();
        explicit SimulationReport(SimTK::State&&);
        SimulationReport(SimTK::State&&, std::unordered_map<UID, float> auxiliary_values);
        SimulationReport(const SimulationReport&);
        SimulationReport(SimulationReport&&) noexcept;
        SimulationReport& operator=(const SimulationReport&);
        SimulationReport& operator=(SimulationReport&&) noexcept;
        ~SimulationReport() noexcept override;

        SimulationClock::time_point get_time() const;
        SimTK::State& upd_state_hack();  // necessary because of a bug in OpenSim PathWrap

        friend bool operator==(const SimulationReport&, const SimulationReport&) = default;

    private:
        const SimTK::State& impl_get_state() const override;
        std::optional<float> impl_get_auxiliary_value(UID) const override;

        class Impl;
        std::shared_ptr<Impl> impl_;
    };
}
