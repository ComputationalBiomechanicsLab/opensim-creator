#pragma once

#include "src/OpenSimBindings/Output.hpp"
#include "src/OpenSimBindings/SimulationClock.hpp"
#include "src/Utils/UID.hpp"

#include <optional>
#include <memory>

namespace OpenSim
{
    class Model;
}

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
        explicit SimulationReport(OpenSim::Model const&, SimTK::State);
        SimulationReport(SimTK::MultibodySystem const&, SimTK::Integrator const&);
        SimulationReport(SimulationReport const&);
        SimulationReport(SimulationReport&&) noexcept;
        SimulationReport& operator=(SimulationReport const&);
        SimulationReport& operator=(SimulationReport&&) noexcept;
        ~SimulationReport() noexcept;

        SimulationClock::time_point getTime() const;
        SimTK::State const& getState() const;
        SimTK::State& updStateHACK();  // necessary because of a bug in OpenSim PathWrap
        std::optional<float> getAuxiliaryValue(UID) const;

        class Impl;
    private:
        friend bool operator==(SimulationReport const&, SimulationReport const&);
        friend bool operator!=(SimulationReport const&, SimulationReport const&);
        std::shared_ptr<Impl> m_Impl;
    };

    inline bool operator==(SimulationReport const& a, SimulationReport const& b)
    {
        return a.m_Impl == b.m_Impl;
    }

    inline bool operator!=(SimulationReport const& a, SimulationReport const& b)
    {
        return a.m_Impl != b.m_Impl;
    }
}
