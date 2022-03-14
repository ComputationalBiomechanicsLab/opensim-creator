#include "SimulationReport.hpp"

#include "src/OpenSimBindings/IntegratorOutput.hpp"
#include "src/OpenSimBindings/MultiBodySystemOutput.hpp"

#include <SimTKsimbody.h>
#include <SimTKcommon.h>
#include <simmath/Integrator.h>

#include <unordered_map>

class osc::SimulationReport::Impl final {
public:

    Impl(SimTK::MultibodySystem const& sys, SimTK::Integrator const& integrator) :
        m_State{integrator.getState()}
    {
        // care: state needs to be realized on the simulator thread

        // populate integrator outputs
        {
            int numOutputs = GetNumIntegratorOutputs();
            m_AuxiliaryValues.reserve(m_AuxiliaryValues.size() + numOutputs);
            for (int i = 0; i < numOutputs; ++i)
            {
                IntegratorOutput const& o = GetIntegratorOutput(i);
                m_AuxiliaryValues.emplace(o.getID(), o.getExtractorFunction()(integrator));
            }
        }

        // populate mbs outputs
        {
            int numOutputs = GetNumMultiBodySystemOutputs();
            m_AuxiliaryValues.reserve(m_AuxiliaryValues.size() + numOutputs);
            for (int i = 0; i < numOutputs; ++i)
            {
                MultiBodySystemOutput const& o = GetMultiBodySystemOutput(i);
                m_AuxiliaryValues.emplace(o.getID(), o.getExtractorFunction()(sys));
            }
        }
    }

    std::unique_ptr<Impl> clone() const
    {
        return std::make_unique<Impl>(*this);
    }

    SimulationClock::time_point getTime() const
    {
        return SimulationClock::start() + SimulationClock::duration{getState().getTime()};
    }

    SimTK::State const& getState() const
    {
        return m_State;
    }

    SimTK::State& updStateHACK()
    {
        return m_State;
    }

    std::optional<float> getAuxiliaryValue(UID id) const
    {
        auto it = m_AuxiliaryValues.find(id);
        return it != m_AuxiliaryValues.end() ? std::optional<float>{it->second} : std::nullopt;
    }

private:
    SimTK::State m_State;
    std::unordered_map<UID, float> m_AuxiliaryValues;
};


// public API

osc::SimulationReport::SimulationReport(SimTK::MultibodySystem const& sys, SimTK::Integrator const& integrator) :
    m_Impl{std::make_shared<Impl>(sys, integrator)}
{
}
osc::SimulationReport::SimulationReport(SimulationReport const&) = default;
osc::SimulationReport::SimulationReport(SimulationReport&&) noexcept = default;
osc::SimulationReport& osc::SimulationReport::operator=(SimulationReport const&) = default;
osc::SimulationReport& osc::SimulationReport::operator=(SimulationReport&&) noexcept = default;
osc::SimulationReport::~SimulationReport() noexcept = default;

osc::SimulationClock::time_point osc::SimulationReport::getTime() const
{
    return m_Impl->getTime();
}

SimTK::State const& osc::SimulationReport::getState() const
{
    return m_Impl->getState();
}

SimTK::State& osc::SimulationReport::updStateHACK()
{
    return m_Impl->updStateHACK();
}

std::optional<float> osc::SimulationReport::getAuxiliaryValue(UID id) const
{
    return m_Impl->getAuxiliaryValue(std::move(id));
}
