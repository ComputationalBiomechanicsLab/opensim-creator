#include "UiFdSimulation.hpp"

#include "src/OpenSimBindings/BasicModelStatePair.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/FdSimulation.hpp"
#include "src/OpenSimBindings/StateModifications.hpp"
#include "src/OpenSimBindings/UiModel.hpp"
#include "src/Assertions.hpp"

#include <OpenSim/Simulation/Model/Model.h>
#include <SimTKcommon.h>

#include <memory>
#include <vector>

class osc::UiFdSimulation::Impl final {
public:

    Impl(BasicModelStatePair, FdParams const&)
    {
        todo
    }

    OpenSim::Model const& getModel() const
    {

    }

    UID getModelVersion() const
    {

    }

    int getNumReports() const
    {

    }

    SimulationReport const& getSimulationReport(int reportIndex)
    {
    }

    SimTK::State const& getReportState(int reportIndex)
    {
    }

    int tryGetAllReportNumericValues(Output const&, std::vector<float>& appendOut) const
    {
    }

    std::optional<std::string> tryGetOutputString(Output const&, int reportIndex) const
    {
    }

    SimulationStatus getSimulationStatus() const
    {
    }

    void requestStop()
    {
    }

    void stop()
    {
    }

    std::chrono::duration<double> getSimulationEndTime() const
    {
    }

    float getSimulationProgress() const
    {

    }

    ParamBlock const& getSimulationParams() const
    {
    }

    nonstd::span<Output const> getOutputs() const
    {
    }

private:
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

osc::UID osc::UiFdSimulation::getModelVersion() const
{
    return m_Impl->getModelVersion();
}

int osc::UiFdSimulation::getNumReports() const
{
    return m_Impl->getNumReports();
}

osc::SimulationReport const& osc::UiFdSimulation::getSimulationReport(int reportIndex)
{
    return m_Impl->getSimulationReport(std::move(reportIndex));
}

SimTK::State const& osc::UiFdSimulation::getReportState(int reportIndex)
{
    return m_Impl->getReportState(std::move(reportIndex));
}

int osc::UiFdSimulation::tryGetAllReportNumericValues(Output const& output, std::vector<float>& appendOut) const
{
    return m_Impl->tryGetAllReportNumericValues(output, appendOut);
}

std::optional<std::string> osc::UiFdSimulation::tryGetOutputString(Output const& output, int reportIndex) const
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

std::chrono::duration<double> osc::UiFdSimulation::getSimulationEndTime() const
{
    return m_Impl->getSimulationEndTime();
}

float osc::UiFdSimulation::getSimulationProgress() const
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
