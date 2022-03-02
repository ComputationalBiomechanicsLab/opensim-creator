#include "SimulationReport.hpp"

#include "src/OpenSimBindings/IntegratorOutput.hpp"

#include <SimTKcommon.h>
#include <simmath/Integrator.h>

#include <unordered_map>

class osc::SimulationReport::Impl final {
public:

    Impl(SimTK::Integrator const& integrator) :
        m_State{integrator.getState()}
    {
        int numOutputs = GetNumIntegratorOutputs();
        m_AuxiliaryValues.reserve(numOutputs);
        for (int i = 0; i < numOutputs; ++i)
        {
            IntegratorOutput const& o = GetIntegratorOutput(i);
            m_AuxiliaryValues.emplace(o.getID(), o.getExtractorFunction()(integrator));
        }
    }

    std::unique_ptr<Impl> clone() const
    {
        return std::make_unique<Impl>(*this);
    }

    SimTK::State const& getState() const
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

osc::SimulationReport::SimulationReport(SimTK::MultibodySystem const&, SimTK::Integrator const& integrator) :
    m_Impl{std::make_shared<Impl>(integrator)}
{
}
osc::SimulationReport::SimulationReport(SimulationReport const&) = default;
osc::SimulationReport::SimulationReport(SimulationReport&&) noexcept = default;
osc::SimulationReport& osc::SimulationReport::operator=(SimulationReport const&) = default;
osc::SimulationReport& osc::SimulationReport::operator=(SimulationReport&&) noexcept = default;
osc::SimulationReport::~SimulationReport() noexcept = default;

SimTK::State const& osc::SimulationReport::getState() const
{
    return m_Impl->getState();
}

std::optional<float> osc::SimulationReport::getAuxiliaryValue(UID id) const
{
    return m_Impl->getAuxiliaryValue(std::move(id));
}
