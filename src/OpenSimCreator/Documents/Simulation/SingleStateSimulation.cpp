#include "SingleStateSimulation.h"

#include <OpenSimCreator/Documents/Model/BasicModelStatePair.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>
#include <OpenSimCreator/Utils/ParamBlock.h>
#include <oscar/Utils/SynchronizedValue.h>
#include <oscar/Utils/SynchronizedValueGuard.h>

using namespace osc;

class osc::SingleStateSimulation::Impl final {
public:
    explicit Impl(BasicModelStatePair modelState) :
        m_ModelState{std::move(modelState)}
    {}

    SynchronizedValueGuard<const OpenSim::Model> getModel() const
    {
        return m_ModelState.lock_child<OpenSim::Model>([](const BasicModelStatePair& ms) -> const OpenSim::Model& { return ms.getModel(); });
    }

    ptrdiff_t getNumReports() const
    {
        return 0;
    }

    SimulationReport getSimulationReport(ptrdiff_t) const
    {
        throw std::runtime_error{"invalid method call on a SingleStateSimulation"};
    }

    std::vector<SimulationReport> getAllSimulationReports() const
    {
        return {};
    }

    SimulationStatus getStatus() const
    {
        return SimulationStatus::Completed;
    }

    SimulationClocks getClocks() const
    {
        return SimulationClocks{SimulationClock::start()};
    }

    const ParamBlock& getParams() const
    {
        return m_Params;
    }

    std::span<const OutputExtractor> getOutputExtractors() const
    {
        return {};
    }

    float getFixupScaleFactor() const
    {
        return m_ModelState.lock()->getFixupScaleFactor();
    }

    void setFixupScaleFactor(float v)
    {
        return m_ModelState.lock()->setFixupScaleFactor(v);
    }

    std::shared_ptr<Environment> implUpdAssociatedEnvironment()
    {
        return m_ModelState.lock()->tryUpdEnvironment();
    }

private:
    SynchronizedValue<BasicModelStatePair> m_ModelState;
    ParamBlock m_Params;
};


osc::SingleStateSimulation::SingleStateSimulation(BasicModelStatePair modelState) :
    m_Impl{std::make_unique<Impl>(std::move(modelState))}
{}
osc::SingleStateSimulation::SingleStateSimulation(SingleStateSimulation&&) noexcept = default;
osc::SingleStateSimulation& osc::SingleStateSimulation::operator=(SingleStateSimulation&&) noexcept = default;
osc::SingleStateSimulation::~SingleStateSimulation() noexcept = default;

SynchronizedValueGuard<const OpenSim::Model> osc::SingleStateSimulation::implGetModel() const
{
    return m_Impl->getModel();
}

ptrdiff_t osc::SingleStateSimulation::implGetNumReports() const
{
    return m_Impl->getNumReports();
}

SimulationReport osc::SingleStateSimulation::implGetSimulationReport(ptrdiff_t reportIndex) const
{
    return m_Impl->getSimulationReport(reportIndex);
}

std::vector<SimulationReport> osc::SingleStateSimulation::implGetAllSimulationReports() const
{
    return m_Impl->getAllSimulationReports();
}

SimulationStatus osc::SingleStateSimulation::implGetStatus() const
{
    return m_Impl->getStatus();
}

SimulationClocks osc::SingleStateSimulation::implGetClocks() const
{
    return m_Impl->getClocks();
}

const ParamBlock& osc::SingleStateSimulation::implGetParams() const
{
    return m_Impl->getParams();
}

std::span<const OutputExtractor> osc::SingleStateSimulation::implGetOutputExtractors() const
{
    return m_Impl->getOutputExtractors();
}

float osc::SingleStateSimulation::implGetFixupScaleFactor() const
{
    return m_Impl->getFixupScaleFactor();
}

void osc::SingleStateSimulation::implSetFixupScaleFactor(float v)
{
    m_Impl->setFixupScaleFactor(v);
}

std::shared_ptr<Environment> osc::SingleStateSimulation::implUpdAssociatedEnvironment()
{
    return m_Impl->implUpdAssociatedEnvironment();
}
