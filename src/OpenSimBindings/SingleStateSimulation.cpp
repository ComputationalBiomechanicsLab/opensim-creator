#include "SingleStateSimulation.hpp"

#include "src/OpenSimBindings/BasicModelStatePair.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/ParamBlock.hpp"

#include <Simbody.h>

class osc::SingleStateSimulation::Impl final {
public:
    Impl(BasicModelStatePair modelState) :
        m_ModelState{std::move(modelState)}
    {
    }

    SynchronizedValueGuard<OpenSim::Model const> getModel() const
    {
        return m_ModelState.lockChild<OpenSim::Model>([](BasicModelStatePair const& ms) -> OpenSim::Model const& { return ms.getModel(); });
    }

    int getNumReports() const
    {
        return 0;
    }

    SimulationReport getSimulationReport(int reportIndex) const
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

    SimulationClock::time_point getCurTime() const
    {
        return SimulationClock::start();
    }

    SimulationClock::time_point getStartTime() const
    {
        return SimulationClock::start();
    }

    SimulationClock::time_point getEndTime() const
    {
        return SimulationClock::start();
    }

    float getProgress() const
    {
        return 1.0f;
    }

    ParamBlock const& getParams() const
    {
        return m_Params;
    }

    nonstd::span<OutputExtractor const> getOutputExtractors() const
    {
        return {};
    }

    void requestStop()
    {
    }

    void stop()
    {
    }

    float getFixupScaleFactor() const
    {
        return m_ModelState.lock()->getFixupScaleFactor();
    }

    void setFixupScaleFactor(float v)
    {
        return m_ModelState.lock()->setFixupScaleFactor(v);
    }

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

osc::SynchronizedValueGuard<OpenSim::Model const> osc::SingleStateSimulation::implGetModel() const
{
    return m_Impl->getModel();
}

int osc::SingleStateSimulation::implGetNumReports() const
{
    return m_Impl->getNumReports();
}

osc::SimulationReport osc::SingleStateSimulation::implGetSimulationReport(int reportIndex) const
{
    return m_Impl->getSimulationReport(reportIndex);
}

std::vector<osc::SimulationReport> osc::SingleStateSimulation::implGetAllSimulationReports() const
{
    return m_Impl->getAllSimulationReports();
}

osc::SimulationStatus osc::SingleStateSimulation::implGetStatus() const
{
    return m_Impl->getStatus();
}

osc::SimulationClock::time_point osc::SingleStateSimulation::implGetCurTime() const
{
    return m_Impl->getCurTime();
}

osc::SimulationClock::time_point osc::SingleStateSimulation::implGetStartTime() const
{
    return m_Impl->getStartTime();
}

osc::SimulationClock::time_point osc::SingleStateSimulation::implGetEndTime() const
{
    return m_Impl->getEndTime();
}

float osc::SingleStateSimulation::implGetProgress() const
{
    return m_Impl->getProgress();
}

osc::ParamBlock const& osc::SingleStateSimulation::implGetParams() const
{
    return m_Impl->getParams();
}

nonstd::span<osc::OutputExtractor const> osc::SingleStateSimulation::implGetOutputExtractors() const
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