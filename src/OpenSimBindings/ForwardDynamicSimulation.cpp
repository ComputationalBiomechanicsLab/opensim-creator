#include "ForwardDynamicSimulation.hpp"

#include "src/OpenSimBindings/BasicModelStatePair.hpp"
#include "src/OpenSimBindings/ForwardDynamicSimulator.hpp"
#include "src/OpenSimBindings/ForwardDynamicSimulatorParams.hpp"
#include "src/OpenSimBindings/OutputExtractor.hpp"
#include "src/OpenSimBindings/ParamBlock.hpp"
#include "src/OpenSimBindings/SimulationClock.hpp"
#include "src/OpenSimBindings/SimulationReport.hpp"
#include "src/OpenSimBindings/SimulationStatus.hpp"
#include "src/Utils/SynchronizedValue.hpp"

#include <nonstd/span.hpp>
#include <OpenSim/Simulation/Model/Model.h>
#include <SimTKcommon.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

// helpers
namespace
{
    // creates a simulator that's hooked up to the reports vector
    osc::ForwardDynamicSimulator MakeSimulation(
        osc::BasicModelStatePair p,
        osc::ForwardDynamicSimulatorParams const& params,
        osc::SynchronizedValue<std::vector<osc::SimulationReport>>& reportQueue)
    {
        auto callback = [&](osc::SimulationReport r)
        {
            auto reportsGuard = reportQueue.lock();
            reportsGuard->push_back(std::move(r));
        };
        return osc::ForwardDynamicSimulator{std::move(p), params, std::move(callback)};
    }

    std::vector<osc::OutputExtractor> GetFdSimulatorOutputExtractorsAsVector()
    {
        std::vector<osc::OutputExtractor> rv;
        int nOutputExtractors = osc::GetNumFdSimulatorOutputExtractors();
        rv.reserve(nOutputExtractors);
        for (int i = 0; i < nOutputExtractors; ++i)
        {
            rv.push_back(osc::GetFdSimulatorOutputExtractor(i));
        }
        return rv;
    }
}

class osc::ForwardDynamicSimulation::Impl final {
public:

    Impl(BasicModelStatePair p, ForwardDynamicSimulatorParams const& params) :
        m_ModelState{std::move(p)},
        m_Simulation{MakeSimulation(*m_ModelState.lock(), params, m_ReportQueue)},
        m_ParamsAsParamBlock{ToParamBlock(params)},
        m_SimulatorOutputExtractors(GetFdSimulatorOutputExtractorsAsVector())
    {
    }

    SynchronizedValueGuard<OpenSim::Model const> getModel() const
    {
        return m_ModelState.lockChild<OpenSim::Model>([](BasicModelStatePair const& p) -> decltype(auto) { return p.getModel(); });
    }

    int getNumReports() const
    {
        popReportsHACK();
        return static_cast<int>(m_Reports.size());
    }

    SimulationReport getSimulationReport(int reportIndex) const
    {
        popReportsHACK();
        return m_Reports.at(reportIndex);
    }

    std::vector<SimulationReport> getAllSimulationReports() const
    {
        popReportsHACK();
        return m_Reports;
    }

    SimulationStatus getStatus() const
    {
        return m_Simulation.getStatus();
    }

    SimulationClock::time_point getCurTime() const
    {
        popReportsHACK();

        if (!m_Reports.empty())
        {
            return SimulationClock::start() + SimulationClock::duration{m_Reports.back().getState().getTime()};
        }
        else
        {
            return getStartTime();
        }
    }

    SimulationClock::time_point getStartTime() const
    {
        return SimulationClock::start() + SimulationClock::duration{m_ModelState.lock()->getState().getTime()};
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

    nonstd::span<OutputExtractor const> getOutputExtractors() const
    {
        return m_SimulatorOutputExtractors;
    }

    void requestStop()
    {
        m_Simulation.requestStop();
    }

    void stop()
    {
        m_Simulation.stop();
    }

    float getFixupScaleFactor() const
    {
        return m_ModelState.lock()->getFixupScaleFactor();
    }

    void setFixupScaleFactor(float v)
    {
        m_ModelState.lock()->setFixupScaleFactor(v);
    }

private:
    // MUST be done from the UI thread
    //
    // the reason this insane hack is necessary is because the background thread
    // requires access to the UI thread's copy of the model in order to perform
    // the realization step
    void popReportsHACK() const
    {
        auto& reports = const_cast<std::vector<SimulationReport>&>(m_Reports);

        std::size_t nReportsBefore = reports.size();

        // pop them onto the local reports queue
        {
            auto guard = const_cast<SynchronizedValue<std::vector<SimulationReport>>&>(m_ReportQueue).lock();

            reports.reserve(reports.size() + guard->size());
            std::copy(std::make_move_iterator(guard->begin()),
                      std::make_move_iterator(guard->end()),
                      std::back_inserter(reports));
            guard->clear();
        }

        std::size_t nReportsAfter = reports.size();
        std::size_t nAdded = nReportsAfter-nReportsBefore;

        if (nAdded <= 0)
        {
            return;
        }

        // ensure all reports are realized on the UI model
        auto modelLock = m_ModelState.lock();
        for (auto it = reports.begin() + nReportsBefore; it != reports.end(); ++it)
        {
            modelLock->getModel().realizeReport(it->updStateHACK());
        }
    }

    SynchronizedValue<BasicModelStatePair> m_ModelState;
    SynchronizedValue<std::vector<SimulationReport>> m_ReportQueue;
    std::vector<SimulationReport> m_Reports;
    ForwardDynamicSimulator m_Simulation;
    ParamBlock m_ParamsAsParamBlock;
    std::vector<OutputExtractor> m_SimulatorOutputExtractors;
};


// public API

osc::ForwardDynamicSimulation::ForwardDynamicSimulation(BasicModelStatePair ms, ForwardDynamicSimulatorParams const& params) :
    m_Impl{std::make_unique<Impl>(std::move(ms), params)}
{
}

osc::ForwardDynamicSimulation::ForwardDynamicSimulation(ForwardDynamicSimulation&&) noexcept = default;
osc::ForwardDynamicSimulation& osc::ForwardDynamicSimulation::operator=(ForwardDynamicSimulation&&) noexcept = default;
osc::ForwardDynamicSimulation::~ForwardDynamicSimulation() noexcept = default;

osc::SynchronizedValueGuard<OpenSim::Model const> osc::ForwardDynamicSimulation::getModel() const
{
    return m_Impl->getModel();
}

int osc::ForwardDynamicSimulation::getNumReports() const
{
    return m_Impl->getNumReports();
}

osc::SimulationReport osc::ForwardDynamicSimulation::getSimulationReport(int reportIndex) const
{
    return m_Impl->getSimulationReport(std::move(reportIndex));
}

std::vector<osc::SimulationReport> osc::ForwardDynamicSimulation::getAllSimulationReports() const
{
    return m_Impl->getAllSimulationReports();
}

osc::SimulationStatus osc::ForwardDynamicSimulation::getStatus() const
{
    return m_Impl->getStatus();
}

osc::SimulationClock::time_point osc::ForwardDynamicSimulation::getCurTime() const
{
    return m_Impl->getCurTime();
}

osc::SimulationClock::time_point osc::ForwardDynamicSimulation::getStartTime() const
{
    return m_Impl->getStartTime();
}

osc::SimulationClock::time_point osc::ForwardDynamicSimulation::getEndTime() const
{
    return m_Impl->getEndTime();
}

float osc::ForwardDynamicSimulation::getProgress() const
{
    return m_Impl->getProgress();
}

osc::ParamBlock const& osc::ForwardDynamicSimulation::getParams() const
{
    return m_Impl->getParams();
}

nonstd::span<osc::OutputExtractor const> osc::ForwardDynamicSimulation::getOutputExtractors() const
{
    return m_Impl->getOutputExtractors();
}

void osc::ForwardDynamicSimulation::requestStop()
{
    return m_Impl->requestStop();
}

void osc::ForwardDynamicSimulation::stop()
{
    return m_Impl->stop();
}

float osc::ForwardDynamicSimulation::getFixupScaleFactor() const
{
    return m_Impl->getFixupScaleFactor();
}

void osc::ForwardDynamicSimulation::setFixupScaleFactor(float v)
{
    m_Impl->setFixupScaleFactor(std::move(v));
}
