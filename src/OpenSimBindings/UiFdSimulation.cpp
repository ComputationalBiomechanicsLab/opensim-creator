#include "UiFdSimulation.hpp"

#include "src/OpenSimBindings/BasicModelStatePair.hpp"
#include "src/OpenSimBindings/SimulationClock.hpp"
#include "src/OpenSimBindings/SimulationReport.hpp"
#include "src/OpenSimBindings/SimulationStatus.hpp"
#include "src/OpenSimBindings/VirtualSimulation.hpp"
#include "src/OpenSimBindings/FdSimulation.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/UID.hpp"
#include "src/Utils/SynchronizedValue.hpp"
#include "src/App.hpp"

#include <nonstd/span.hpp>
#include <OpenSim/Simulation/Model/Model.h>

#include <mutex>
#include <optional>
#include <memory>
#include <vector>

// helper function for creating a simulator that's hooked up to the reports vector
static osc::FdSimulation MakeSimulation(
        osc::BasicModelStatePair p,
        osc::FdParams const& params,
        osc::SynchronizedValue<std::vector<osc::SimulationReport>>& reports)
{
    auto callback = [&](osc::SimulationReport r)
    {
        reports.lock()->push_back(std::move(r));
        osc::App::cur().requestRedraw();
    };
    return osc::FdSimulation{std::move(p), params, std::move(callback)};
}

static std::vector<osc::Output> GetFdSimulatorOutputsAsVector()
{
    std::vector<osc::Output> rv;
    int nOutputs = osc::GetNumFdSimulatorOutputs();
    rv.reserve(nOutputs);
    for (int i = 0; i < nOutputs; ++i)
    {
        rv.push_back(osc::GetFdSimulatorOutputDynamic(i));
    }
    return rv;
}

class osc::UiFdSimulation::Impl final {
public:

    Impl(BasicModelStatePair p, FdParams const& params) :
        m_Simulation{MakeSimulation(p, params, m_Reports)},
        m_ModelState{std::move(p)},
        m_ParamsAsParamBlock{ToParamBlock(params)},
        m_SimulatorOutputs(GetFdSimulatorOutputsAsVector())
    {
    }

    OpenSim::Model const& getModel() const
    {
        return m_ModelState.getModel();
    }

    int getNumReports() const
    {
        return static_cast<int>(m_Reports.lock()->size());
    }

    SimulationReport getSimulationReport(int reportIndex) const
    {
        return m_Reports.lock()->at(reportIndex);
    }

    std::vector<SimulationReport> getAllSimulationReports() const
    {
        return *m_Reports.lock();
    }

    SimulationStatus getStatus() const
    {
        return m_Simulation.getStatus();
    }

    SimulationClock::time_point getCurTime() const
    {
        auto guard = m_Reports.lock();

        if (!guard->empty())
        {
            return SimulationClock::start() + SimulationClock::duration{guard->back().getState().getTime()};
        }
        else
        {
            return getStartTime();
        }
    }

    SimulationClock::time_point getStartTime() const
    {
        return SimulationClock::start() + SimulationClock::duration{m_ModelState.getState().getTime()};
    }

    SimulationClock::time_point getEndTime() const
    {
        return m_Simulation.params().FinalTime;
    }

    float getProgress() const
    {
        auto start = getStartTime();
        auto end = getEndTime();
        auto cur = getCurTime();
        return static_cast<float>((cur-start)/(end-start));
    }

    ParamBlock const& getParams() const
    {
        return m_ParamsAsParamBlock;
    }

    nonstd::span<Output const> getOutputs() const
    {
        return m_SimulatorOutputs;
    }

    void requestStop()
    {
        m_Simulation.requestStop();
    }

    void stop()
    {
        m_Simulation.stop();
    }

private:
    SynchronizedValue<std::vector<SimulationReport>> m_Reports;
    FdSimulation m_Simulation;
    BasicModelStatePair m_ModelState;
    ParamBlock m_ParamsAsParamBlock;
    std::vector<Output> m_SimulatorOutputs;
};


// public API

osc::UiFdSimulation::UiFdSimulation(BasicModelStatePair ms,FdParams const& params) :
    m_Impl{std::make_unique<Impl>(std::move(ms), params)}
{
}
osc::UiFdSimulation::UiFdSimulation(UiFdSimulation&&) noexcept = default;
osc::UiFdSimulation& osc::UiFdSimulation::operator=(UiFdSimulation&&) noexcept = default;
osc::UiFdSimulation::~UiFdSimulation() noexcept = default;

OpenSim::Model const& osc::UiFdSimulation::getModel() const
{
    return m_Impl->getModel();
}

int osc::UiFdSimulation::getNumReports() const
{
    return m_Impl->getNumReports();
}

osc::SimulationReport osc::UiFdSimulation::getSimulationReport(int reportIndex) const
{
    return m_Impl->getSimulationReport(std::move(reportIndex));
}

std::vector<osc::SimulationReport> osc::UiFdSimulation::getAllSimulationReports() const
{
    return m_Impl->getAllSimulationReports();
}

osc::SimulationStatus osc::UiFdSimulation::getStatus() const
{
    return m_Impl->getStatus();
}

osc::SimulationClock::time_point osc::UiFdSimulation::getCurTime() const
{
    return m_Impl->getCurTime();
}

osc::SimulationClock::time_point osc::UiFdSimulation::getStartTime() const
{
    return m_Impl->getStartTime();
}

osc::SimulationClock::time_point osc::UiFdSimulation::getEndTime() const
{
    return m_Impl->getEndTime();
}

float osc::UiFdSimulation::getProgress() const
{
    return m_Impl->getProgress();
}

osc::ParamBlock const& osc::UiFdSimulation::getParams() const
{
    return m_Impl->getParams();
}

nonstd::span<osc::Output const> osc::UiFdSimulation::getOutputs() const
{
    return m_Impl->getOutputs();
}

void osc::UiFdSimulation::requestStop()
{
    return m_Impl->requestStop();
}

void osc::UiFdSimulation::stop()
{
    return m_Impl->stop();
}
