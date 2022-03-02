#pragma once

#include "src/OpenSimBindings/Output.hpp"
#include "src/Utils/UID.hpp"

#include <optional>
#include <memory>

namespace SimTK
{
    class MultibodySystem;
    class Integrator;
    class State;
}

namespace osc
{
    // reference-counted, immutable, simulation report
    class SimulationReport {
    public:
        SimulationReport(SimTK::MultibodySystem const&, SimTK::Integrator const&);
        SimulationReport(SimulationReport const&);
        SimulationReport(SimulationReport&&) noexcept;
        SimulationReport& operator=(SimulationReport const&);
        SimulationReport& operator=(SimulationReport&&) noexcept;
        ~SimulationReport() noexcept;

        SimTK::State const& getState() const;
        std::optional<float> getAuxiliaryValue(UID) const;

        class Impl;
    private:
        std::shared_ptr<Impl> m_Impl;
    };
}
