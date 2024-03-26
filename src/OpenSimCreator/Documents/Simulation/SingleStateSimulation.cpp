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

    SynchronizedValueGuard<OpenSim::Model const> getModel() const
    {
        return m_ModelState.lockChild<OpenSim::Model>([](BasicModelStatePair const& ms) -> OpenSim::Model const& { return ms.getModel(); });
    }

    SimulationReportSequence getReports() const
    {
        throw std::runtime_error{"invalid method call on a SingleStateSimulation"};
    }

    SimulationStatus getStatus() const { return SimulationStatus::Completed; }
    SimulationClocks getClocks() const { return SimulationClocks{SimulationClock::start()}; }
    ParamBlock const& getParams() const { return m_Params; }

    std::span<OutputExtractor const> getOutputExtractors() const { return {}; }

    void requestStop() {}
    void stop() {}

    float getFixupScaleFactor() const { return m_ModelState.lock()->getFixupScaleFactor(); }
    void setFixupScaleFactor(float v) { return m_ModelState.lock()->setFixupScaleFactor(v); }

private:
    SynchronizedValue<BasicModelStatePair> m_ModelState;
    ParamBlock m_Params;
};


// public API (PIMPL)

osc::SingleStateSimulation::SingleStateSimulation(BasicModelStatePair modelState) :
    m_Impl{std::make_unique<Impl>(std::move(modelState))}
{
}
osc::SingleStateSimulation::SingleStateSimulation(SingleStateSimulation&&) noexcept = default;
osc::SingleStateSimulation& osc::SingleStateSimulation::operator=(SingleStateSimulation&&) noexcept = default;
osc::SingleStateSimulation::~SingleStateSimulation() noexcept = default;

SynchronizedValueGuard<OpenSim::Model const> osc::SingleStateSimulation::implGetModel() const
{
    return m_Impl->getModel();
}

SimulationReportSequence osc::SingleStateSimulation::implGetReports() const
{
    return m_Impl->getReports();
}

SimulationStatus osc::SingleStateSimulation::implGetStatus() const
{
    return m_Impl->getStatus();
}

SimulationClocks osc::SingleStateSimulation::implGetClocks() const
{
    return m_Impl->getClocks();
}

ParamBlock const& osc::SingleStateSimulation::implGetParams() const
{
    return m_Impl->getParams();
}

std::span<OutputExtractor const> osc::SingleStateSimulation::implGetOutputExtractors() const
{
    return m_Impl->getOutputExtractors();
}

void osc::SingleStateSimulation::implRequestStop()
{
    m_Impl->requestStop();
}

void osc::SingleStateSimulation::implStop()
{
    m_Impl->stop();
}

float osc::SingleStateSimulation::implGetFixupScaleFactor() const
{
    return m_Impl->getFixupScaleFactor();
}

void osc::SingleStateSimulation::implSetFixupScaleFactor(float v)
{
    m_Impl->setFixupScaleFactor(v);
}
