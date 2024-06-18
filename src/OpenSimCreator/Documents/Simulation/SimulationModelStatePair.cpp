#include "SimulationModelStatePair.h"

#include <OpenSimCreator/Documents/Model/BasicModelStatePair.h>
#include <OpenSimCreator/Documents/Simulation/Simulation.h>
#include <OpenSimCreator/Documents/Simulation/SimulationReport.h>
#include <OpenSimCreator/Documents/Simulation/SingleStateSimulation.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <oscar/Utils/UID.h>

#include <memory>
#include <utility>

using namespace osc;

class osc::SimulationModelStatePair::Impl final {
public:
    Impl() :
        m_Simulation{std::make_shared<Simulation>(SingleStateSimulation{BasicModelStatePair{}})}
    {}

    Impl(std::shared_ptr<Simulation> simulation, SimulationReport simulationReport) :
        m_Simulation{std::move(simulation)},
        m_SimulationReport{std::move(simulationReport)}
    {}

    const OpenSim::Model& getModel() const
    {
        return *m_Simulation->getModel();  // TODO: UH OH - lock leak (#707 - are locks necessary?)
    }

    UID getModelVersion() const
    {
        return m_ModelVersion;
    }

    const SimTK::State& getState() const
    {
        return m_SimulationReport.getState();
    }

    UID getStateVersion() const
    {
        return m_StateVersion;
    }

    OpenSim::Component const* getSelected() const
    {
        return FindComponent(getModel(), m_Selected);
    }

    void setSelected(OpenSim::Component const* c)
    {
        m_Selected = GetAbsolutePathOrEmpty(c);
    }

    OpenSim::Component const* getHovered() const
    {
        return FindComponent(getModel(), m_Hovered);
    }

    void setHovered(OpenSim::Component const* c)
    {
        m_Hovered = GetAbsolutePathOrEmpty(c);
    }

    float getFixupScaleFactor() const
    {
        return m_Simulation->getFixupScaleFactor();
    }

    void setFixupScaleFactor(float v)
    {
        m_Simulation->setFixupScaleFactor(v);
    }

    std::shared_ptr<Simulation> updSimulation()
    {
        return m_Simulation;
    }

    void setSimulation(std::shared_ptr<Simulation> s)
    {
        if (s != m_Simulation)
        {
            m_Simulation = std::move(s);
            m_ModelVersion = UID{};
        }
    }

    SimulationReport getSimulationReport() const
    {
        return m_SimulationReport;
    }

    void setSimulationReport(SimulationReport r)
    {
        if (r != m_SimulationReport)
        {
            m_SimulationReport = std::move(r);
            m_StateVersion = UID{};
        }
    }

private:
    UID m_ModelVersion;
    UID m_StateVersion;
    OpenSim::ComponentPath m_Selected;
    OpenSim::ComponentPath m_Hovered;
    std::shared_ptr<Simulation> m_Simulation;
    SimulationReport m_SimulationReport;
};


// public API (PIMPL)

osc::SimulationModelStatePair::SimulationModelStatePair() :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::SimulationModelStatePair::SimulationModelStatePair(std::shared_ptr<Simulation> simulation, SimulationReport report) :
    m_Impl{std::make_unique<Impl>(std::move(simulation), std::move(report))}
{
}

osc::SimulationModelStatePair::SimulationModelStatePair(SimulationModelStatePair&&) noexcept = default;
osc::SimulationModelStatePair& osc::SimulationModelStatePair::operator=(SimulationModelStatePair&&) noexcept = default;
osc::SimulationModelStatePair::~SimulationModelStatePair() noexcept = default;

std::shared_ptr<Simulation> osc::SimulationModelStatePair::updSimulation()
{
    return m_Impl->updSimulation();
}

void osc::SimulationModelStatePair::setSimulation(std::shared_ptr<Simulation> sim)
{
    m_Impl->setSimulation(std::move(sim));
}

SimulationReport osc::SimulationModelStatePair::getSimulationReport() const
{
    return m_Impl->getSimulationReport();
}

void osc::SimulationModelStatePair::setSimulationReport(SimulationReport report)
{
    m_Impl->setSimulationReport(std::move(report));
}

const OpenSim::Model& osc::SimulationModelStatePair::implGetModel() const
{
    return m_Impl->getModel();
}

UID osc::SimulationModelStatePair::implGetModelVersion() const
{
    return m_Impl->getModelVersion();
}

const SimTK::State& osc::SimulationModelStatePair::implGetState() const
{
    return m_Impl->getState();
}

UID osc::SimulationModelStatePair::implGetStateVersion() const
{
    return m_Impl->getStateVersion();
}

OpenSim::Component const* osc::SimulationModelStatePair::implGetSelected() const
{
    return m_Impl->getSelected();
}

void osc::SimulationModelStatePair::implSetSelected(OpenSim::Component const* c)
{
    m_Impl->setSelected(c);
}

OpenSim::Component const* osc::SimulationModelStatePair::implGetHovered() const
{
    return m_Impl->getHovered();
}

void osc::SimulationModelStatePair::implSetHovered(OpenSim::Component const* c)
{
    m_Impl->setHovered(c);
}

float osc::SimulationModelStatePair::implGetFixupScaleFactor() const
{
    return m_Impl->getFixupScaleFactor();
}

void osc::SimulationModelStatePair::implSetFixupScaleFactor(float v)
{
    m_Impl->setFixupScaleFactor(v);
}
