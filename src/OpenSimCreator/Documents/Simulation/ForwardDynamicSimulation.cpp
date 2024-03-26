#include "ForwardDynamicSimulation.h"

#include <OpenSimCreator/Documents/Model/BasicModelStatePair.h>
#include <OpenSimCreator/Documents/Simulation/ForwardDynamicSimulator.h>
#include <OpenSimCreator/Documents/Simulation/ForwardDynamicSimulatorParams.h>
#include <OpenSimCreator/Documents/Simulation/SimulationClock.h>
#include <OpenSimCreator/Documents/Simulation/SimulationReport.h>
#include <OpenSimCreator/Documents/Simulation/SimulationStatus.h>
#include <OpenSimCreator/OutputExtractors/OutputExtractor.h>
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
        ForwardDynamicSimulatorParams const& params,
        std::shared_ptr<SynchronizedValue<SimulationReportSequence>>& reports)
    {
        auto callback = [reports](SimulationReport r)
        {
            auto guard = reports->lock();
            guard->emplace_back(std::move(r).stealState(), r.getAllAuxiliaryValuesHACK());
        };
        return ForwardDynamicSimulator{std::move(p), params, std::move(callback)};
    }

    std::vector<OutputExtractor> GetFdSimulatorOutputExtractorsAsVector()
    {
        std::vector<OutputExtractor> rv;
        int const nOutputExtractors = GetNumFdSimulatorOutputExtractors();
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

    Impl(BasicModelStatePair p, ForwardDynamicSimulatorParams const& params) :
        m_ModelState{std::move(p)},
        m_Simulation{MakeSimulation(*m_ModelState.lock(), params, m_Reports)},
        m_ParamsAsParamBlock{ToParamBlock(params)},
        m_SimulatorOutputExtractors(GetFdSimulatorOutputExtractorsAsVector())
    {}

    SynchronizedValueGuard<OpenSim::Model const> getModel() const
    {
        return m_ModelState.lockChild<OpenSim::Model>([](BasicModelStatePair const& p) -> decltype(auto) { return p.getModel(); });
    }

    SimulationReportSequence getReports() const
    {
        return *m_Reports->lock();
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
        auto guard = m_Reports->lock();
        auto size = guard->size();
        if (size > 0) {
            return guard->timeOf(size-1);
        }
        else {
            return getStartTime();
        }
    }

    SimulationClocks getClocks() const
    {
        auto start = getStartTime();
        auto end = m_Simulation.params().finalTime;
        return SimulationClocks{{start, end}, getCurTime()};
    }

    ParamBlock const& getParams() const
    {
        return m_ParamsAsParamBlock;
    }

    std::span<OutputExtractor const> getOutputExtractors() const
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
    SynchronizedValue<BasicModelStatePair> m_ModelState;
    std::shared_ptr<SynchronizedValue<SimulationReportSequence>> m_Reports = std::make_shared<SynchronizedValue<SimulationReportSequence>>();
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

SynchronizedValueGuard<OpenSim::Model const> osc::ForwardDynamicSimulation::implGetModel() const
{
    return m_Impl->getModel();
}

SimulationReportSequence osc::ForwardDynamicSimulation::implGetReports() const
{
    return m_Impl->getReports();
}

SimulationStatus osc::ForwardDynamicSimulation::implGetStatus() const
{
    return m_Impl->getStatus();
}

SimulationClocks osc::ForwardDynamicSimulation::implGetClocks() const
{
    return m_Impl->getClocks();
}

ParamBlock const& osc::ForwardDynamicSimulation::implGetParams() const
{
    return m_Impl->getParams();
}

std::span<OutputExtractor const> osc::ForwardDynamicSimulation::implGetOutputExtractors() const
{
    return m_Impl->getOutputExtractors();
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
