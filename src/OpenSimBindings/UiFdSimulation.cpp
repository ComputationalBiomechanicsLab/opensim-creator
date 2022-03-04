#include "UiFdSimulation.hpp"

#include "src/OpenSimBindings/BasicModelStatePair.hpp"
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

#include <chrono>
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
        m_Simulation{MakeSimulation(p, params, m_QueuedReports)},
        m_ModelState{std::move(p)},
        m_ParamsAsParamBlock{ToParamBlock(params)},
        m_SimulatorOutputs{GetFdSimulatorOutputsAsVector()}
    {
    }

    OpenSim::Model const& getModel() const
    {
        return m_ModelState.getModel();
    }

    int getNumReports()
    {
        popReports();
        return static_cast<int>(m_Reports.size());
    }

    SimulationReport getSimulationReport(int reportIndex)
    {
        popReports();
        return m_Reports.at(reportIndex);
    }

    int tryGetAllReportNumericValues(Output const& o, std::vector<float>& appendOut)
    {
        popReports();

        if (!o.producesNumericValues())
        {
            return 0;
        }

        OpenSim::Model const& model = getModel();

        int numEls = 0;
        for (SimulationReport const& report : m_Reports)
        {
            appendOut.push_back(o.getNumericValue(model, report).value_or(-1337.0f));
            ++numEls;
        }
        return numEls;
    }

    std::optional<std::string> tryGetOutputString(Output const& o, int reportIndex)
    {
        popReports();
        return o.getStringValue(getModel(), getSimulationReport(reportIndex));
    }

    SimulationStatus getSimulationStatus() const
    {
        return m_Simulation.getStatus();
    }

    void requestStop()
    {
        m_Simulation.requestStop();
    }

    void stop()
    {
        m_Simulation.stop();
    }

    std::chrono::duration<double> getSimulationCurTime()
    {
        popReports();

        if (!m_Reports.empty())
        {
            return std::chrono::duration<double>{m_Reports.back().getState().getTime()};
        }
        else
        {
            return std::chrono::duration<double>{0.0};
        }
    }

    std::chrono::duration<double> getSimulationEndTime() const
    {
        return m_Simulation.params().FinalTime;
    }

    float getSimulationProgress()
    {
        double ratio = getSimulationCurTime().count()/getSimulationEndTime().count();
        return static_cast<float>(ratio);
    }

    ParamBlock const& getSimulationParams() const
    {
        return m_ParamsAsParamBlock;
    }

    nonstd::span<Output const> getOutputs() const
    {
        return m_SimulatorOutputs;
    }

private:
    void popReports()
    {
        auto lock = m_QueuedReports.lock();

        if (!lock->empty())
        {
            TransferToEnd(*lock, m_Reports);
            App::cur().requestRedraw();  // HACK: ensure app draws after potential update
        }
    }

    SynchronizedValue<std::vector<SimulationReport>> m_QueuedReports;
    std::vector<SimulationReport> m_Reports;
    FdSimulation m_Simulation;
    BasicModelStatePair m_ModelState;
    ParamBlock m_ParamsAsParamBlock;
    std::vector<Output> m_SimulatorOutputs;
};

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

int osc::UiFdSimulation::getNumReports()
{
    return m_Impl->getNumReports();
}

osc::SimulationReport osc::UiFdSimulation::getSimulationReport(int reportIndex)
{
    return m_Impl->getSimulationReport(std::move(reportIndex));
}

int osc::UiFdSimulation::tryGetAllReportNumericValues(Output const& output, std::vector<float>& appendOut)
{
    return m_Impl->tryGetAllReportNumericValues(output, appendOut);
}

std::optional<std::string> osc::UiFdSimulation::tryGetOutputString(Output const& output, int reportIndex)
{
    return m_Impl->tryGetOutputString(output, std::move(reportIndex));
}

osc::SimulationStatus osc::UiFdSimulation::getSimulationStatus() const
{
    return m_Impl->getSimulationStatus();
}

void osc::UiFdSimulation::requestStop()
{
    return m_Impl->requestStop();
}

void osc::UiFdSimulation::stop()
{
    return m_Impl->stop();
}

std::chrono::duration<double> osc::UiFdSimulation::getSimulationCurTime()
{
    return m_Impl->getSimulationCurTime();
}

std::chrono::duration<double> osc::UiFdSimulation::getSimulationEndTime() const
{
    return m_Impl->getSimulationEndTime();
}

float osc::UiFdSimulation::getSimulationProgress()
{
    return m_Impl->getSimulationProgress();
}

osc::ParamBlock const& osc::UiFdSimulation::getSimulationParams() const
{
    return m_Impl->getSimulationParams();
}

nonstd::span<osc::Output const> osc::UiFdSimulation::getOutputs() const
{
    return m_Impl->getOutputs();
}
