#include "ForwardDynamicSimulation.h"

#include <OpenSimCreator/Documents/Model/BasicModelStatePair.h>
#include <OpenSimCreator/Documents/OutputExtractors/OutputExtractor.h>
#include <OpenSimCreator/Documents/Simulation/ForwardDynamicSimulator.h>
#include <OpenSimCreator/Documents/Simulation/ForwardDynamicSimulatorParams.h>
#include <OpenSimCreator/Documents/Simulation/SimulationClock.h>
#include <OpenSimCreator/Documents/Simulation/SimulationReport.h>
#include <OpenSimCreator/Documents/Simulation/SimulationStatus.h>
#include <OpenSimCreator/Utils/ParamBlock.h>

#include <OpenSim/Simulation/Model/Model.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/SynchronizedValue.h>
#include <oscar/Utils/SynchronizedValueGuard.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

using namespace osc;

// helpers
namespace
{
    // creates a simulator that's hooked up to the reports vector
    ForwardDynamicSimulator MakeSimulation(
        BasicModelStatePair p,
        const ForwardDynamicSimulatorParams& params,
        SynchronizedValue<std::vector<SimulationReport>>& reportQueue)
    {
        auto callback = [&](SimulationReport r)
        {
            auto reportsGuard = reportQueue.lock();
            reportsGuard->push_back(std::move(r));
        };
        return ForwardDynamicSimulator{std::move(p), params, std::move(callback)};
    }

    std::vector<OutputExtractor> GetFdSimulatorOutputExtractorsAsVector()
    {
        std::vector<OutputExtractor> rv;
        const int nOutputExtractors = GetNumFdSimulatorOutputExtractors();
        rv.reserve(nOutputExtractors);
        for (int i = 0; i < nOutputExtractors; ++i)
        {
            rv.push_back(GetFdSimulatorOutputExtractor(i));
        }
        return rv;
    }
}

class osc::ForwardDynamicSimulation::Impl final {
public:

    Impl(BasicModelStatePair p, const ForwardDynamicSimulatorParams& params) :
        m_ModelState{std::move(p)},
        m_Simulation{MakeSimulation(*m_ModelState.lock(), params, m_ReportQueue)},
        m_Params{params},
        m_ParamsAsParamBlock{ToParamBlock(params)},
        m_SimulatorOutputExtractors(GetFdSimulatorOutputExtractorsAsVector())
    {}

    void join()
    {
        m_Simulation.join();
    }

    SynchronizedValueGuard<OpenSim::Model const> getModel() const
    {
        return m_ModelState.lock_child<OpenSim::Model>([](const BasicModelStatePair& p) -> decltype(auto) { return p.getModel(); });
    }

    ptrdiff_t getNumReports() const
    {
        popReportsHACK();
        return m_Reports.size();
    }

    SimulationReport getSimulationReport(ptrdiff_t reportIndex) const
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

    SimulationClock::time_point getStartTime() const
    {
        return SimulationClock::start() + SimulationClock::duration{m_ModelState.lock()->getState().getTime()};
    }

    SimulationClock::time_point getCurTime() const
    {
        popReportsHACK();

        if (not m_Reports.empty()) {
            return SimulationClock::start() + SimulationClock::duration{m_Reports.back().getState().getTime()};
        }
        else {
            return getStartTime();
        }
    }

    SimulationClocks getClocks() const
    {
        auto start = getStartTime();
        auto end = m_Params.finalTime;
        return SimulationClocks{{start, end}, getCurTime()};
    }

    const ParamBlock& getParams() const
    {
        return m_ParamsAsParamBlock;
    }

    std::span<OutputExtractor const> getOutputExtractors() const
    {
        return m_SimulatorOutputExtractors;
    }

    void requestNewEndTime(SimulationClock::time_point new_end_time)
    {
        const SimulationClock::time_point old_end_time = getClocks().end();

        if (new_end_time == old_end_time) {
            return;  // nothing to change
        }

        // before doing anything, sychronously stop current simulation and
        // ensure the report queue is popped
        {
            stop();
            popReportsHACK();
        }

        // if necessary, truncate any dangling reports
        if (new_end_time < old_end_time and not m_Reports.empty()) {

            const auto reportBeforeOrEqualToNewEndTime = [new_end_time](const SimulationReport& r)
            {
                return r.getTime() <= new_end_time;
            };
            const auto it = find_if(m_Reports.rbegin(), m_Reports.rend(), reportBeforeOrEqualToNewEndTime);
            m_Reports.erase(it.base(), m_Reports.end());
        }

        // update the simulation parameters to reflect the new end-time
        {
            m_Params.finalTime = new_end_time;
            m_ParamsAsParamBlock = ToParamBlock(m_Params);
        }

        // edge-case: if the latest available report has an end-time equal to `t`, then
        // our work is complete
        if (not m_Reports.empty() and m_Reports.back().getTime() == new_end_time) {
            return;
        }

        // otherwise, create a new simulator with the new parameters
        {
            const auto guard = m_ModelState.lock();
            const SimTK::State& latestState = m_Reports.empty() ?
                guard->getState() :
                m_Reports.back().getState();

            m_Simulation = MakeSimulation(
                BasicModelStatePair{guard->getModel(), latestState},
                m_Params,
                m_ReportQueue
            );

            // note: the report collection method should ensure that double-reports
            // aren't collected
        }
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

        // handle double-reporting (e.g. due to `requestNewEndTime`) by checking
        // the time of each incoming reports against the latest already collected
        std::optional<SimulationClock::time_point> latestReportTime;
        if (not reports.empty()) {
            latestReportTime = reports.back().getTime();
        }

        const size_t nReportsBefore = reports.size();
        size_t nAdded = 0;

        // pop them onto the local reports queue
        {
            auto guard = const_cast<SynchronizedValue<std::vector<SimulationReport>>&>(m_ReportQueue).lock();
            reports.reserve(reports.size() + guard->size());
            for (SimulationReport& report : *guard) {
                if (report.getTime() == latestReportTime) {
                    continue;  // filter out duplicate reports (e.g. due to `requestNewEndTime`)
                }

                reports.push_back(std::move(report));
                ++nAdded;
            }
            guard->clear();
        }

        if (nAdded <= 0) {
            return;
        }

        // ensure all reports are realized on the UI model
        auto modelLock = m_ModelState.lock();
        for (auto it = reports.begin() + nReportsBefore; it != reports.end(); ++it) {
            modelLock->getModel().realizeReport(it->updStateHACK());
        }
    }

    SynchronizedValue<BasicModelStatePair> m_ModelState;
    SynchronizedValue<std::vector<SimulationReport>> m_ReportQueue;
    std::vector<SimulationReport> m_Reports;
    ForwardDynamicSimulator m_Simulation;
    ForwardDynamicSimulatorParams m_Params;
    ParamBlock m_ParamsAsParamBlock;
    std::vector<OutputExtractor> m_SimulatorOutputExtractors;
};


// public API

osc::ForwardDynamicSimulation::ForwardDynamicSimulation(BasicModelStatePair ms, const ForwardDynamicSimulatorParams& params) :
    m_Impl{std::make_unique<Impl>(std::move(ms), params)}
{}
osc::ForwardDynamicSimulation::ForwardDynamicSimulation(ForwardDynamicSimulation&&) noexcept = default;
osc::ForwardDynamicSimulation& osc::ForwardDynamicSimulation::operator=(ForwardDynamicSimulation&&) noexcept = default;
osc::ForwardDynamicSimulation::~ForwardDynamicSimulation() noexcept = default;

void osc::ForwardDynamicSimulation::join()
{
    m_Impl->join();
}

SynchronizedValueGuard<OpenSim::Model const> osc::ForwardDynamicSimulation::implGetModel() const
{
    return m_Impl->getModel();
}

ptrdiff_t osc::ForwardDynamicSimulation::implGetNumReports() const
{
    return m_Impl->getNumReports();
}

SimulationReport osc::ForwardDynamicSimulation::implGetSimulationReport(ptrdiff_t reportIndex) const
{
    return m_Impl->getSimulationReport(reportIndex);
}

std::vector<SimulationReport> osc::ForwardDynamicSimulation::implGetAllSimulationReports() const
{
    return m_Impl->getAllSimulationReports();
}

SimulationStatus osc::ForwardDynamicSimulation::implGetStatus() const
{
    return m_Impl->getStatus();
}

SimulationClocks osc::ForwardDynamicSimulation::implGetClocks() const
{
    return m_Impl->getClocks();
}

const ParamBlock& osc::ForwardDynamicSimulation::implGetParams() const
{
    return m_Impl->getParams();
}

std::span<OutputExtractor const> osc::ForwardDynamicSimulation::implGetOutputExtractors() const
{
    return m_Impl->getOutputExtractors();
}

void osc::ForwardDynamicSimulation::implRequestNewEndTime(SimulationClock::time_point t)
{
    m_Impl->requestNewEndTime(t);
}

void osc::ForwardDynamicSimulation::implRequestStop()
{
    return m_Impl->requestStop();
}

void osc::ForwardDynamicSimulation::implStop()
{
    return m_Impl->stop();
}

float osc::ForwardDynamicSimulation::implGetFixupScaleFactor() const
{
    return m_Impl->getFixupScaleFactor();
}

void osc::ForwardDynamicSimulation::implSetFixupScaleFactor(float v)
{
    m_Impl->setFixupScaleFactor(v);
}
