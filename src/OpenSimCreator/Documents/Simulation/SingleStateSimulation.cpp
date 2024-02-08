#include "SingleStateSimulation.hpp"

#include <OpenSimCreator/Documents/Model/BasicModelStatePair.hpp>
#include <OpenSimCreator/Utils/OpenSimHelpers.hpp>
#include <OpenSimCreator/Utils/ParamBlock.hpp>
#include <oscar/Utils/SynchronizedValue.hpp>
#include <oscar/Utils/SynchronizedValueGuard.hpp>

using namespace osc;

class osc::SingleStateSimulation::Impl final {
public:
    explicit Impl(BasicModelStatePair modelState) :
        m_ModelState{std::move(modelState)}
    {
    }

    SynchronizedValueGuard<OpenSim::Model const> getModel() const
    {
        return m_ModelState.lockChild<OpenSim::Model>([](BasicModelStatePair const& ms) -> OpenSim::Model const& { return ms.getModel(); });
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

    std::span<OutputExtractor const> getOutputExtractors() const
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

SynchronizedValueGuard<OpenSim::Model const> osc::SingleStateSimulation::implGetModel() const
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

SimulationClock::time_point osc::SingleStateSimulation::implGetCurTime() const
{
    return m_Impl->getCurTime();
}

SimulationClock::time_point osc::SingleStateSimulation::implGetStartTime() const
{
    return m_Impl->getStartTime();
}

SimulationClock::time_point osc::SingleStateSimulation::implGetEndTime() const
{
    return m_Impl->getEndTime();
}

float osc::SingleStateSimulation::implGetProgress() const
{
    return m_Impl->getProgress();
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
