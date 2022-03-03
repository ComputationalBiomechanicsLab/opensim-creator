#pragma once

#include "src/OpenSimBindings/SimulationStatus.hpp"
#include "src/OpenSimBindings/VirtualSimulation.hpp"
#include "src/Utils/UID.hpp"

#include <nonstd/span.hpp>

#include <chrono>
#include <memory>
#include <optional>
#include <vector>
#include <string>

namespace osc
{
    class SimulationReport;
    class Output;
    class ParamBlock;
}

namespace OpenSim
{
    class Model;
}

namespace SimTK
{
    class State;
}
namespace osc
{
    // a "value type" that acts as a container for a osc::VirtualSimulation
    class Simulation final {

        template<class ConcreteSimulation>
        Simulation(ConcreteSimulation&& simulation) :
            m_Simulation{std::make_unique<ConcreteSimulation>(std::forward<ConcreteSimulation>(simulation))}
        {
        }

        OpenSim::Model const& getModel() const { return m_Simulation->getModel(); }
        UID getModelVersion() const { return m_Simulation->getModelVersion(); }
        int getNumReports() const { return m_Simulation->getNumReports(); }
        SimulationReport const& getSimulationReport(int reportIndex) { return m_Simulation->getSimulationReport(std::move(reportIndex)); }
        SimTK::State const& getReportState(int reportIndex) { return m_Simulation->getReportState(std::move(reportIndex)); }
        int tryGetReportNumericValues(Output const& output, std::vector<float>& appendOut) const { return m_Simulation->tryGetAllReportNumericValues(output, appendOut); }
        std::optional<std::string> tryGetOutputString(Output const& output, int reportIndex) const { return m_Simulation->tryGetOutputString(output, std::move(reportIndex)); }
        SimulationStatus getSimulationStatus() const { return m_Simulation->getSimulationStatus(); }
        void requestStop() { m_Simulation->requestStop(); }
        void stop() { m_Simulation->stop(); }
        std::chrono::duration<double> getSimulationEndTime() const { return m_Simulation->getSimulationEndTime(); }
        float getSimulationProgress() const { return m_Simulation->getSimulationProgress(); }
        ParamBlock const& getSimulationParams() const { return m_Simulation->getSimulationParams(); }
        nonstd::span<Output const> getOutputs() const { return m_Simulation->getOutputs(); }

    private:
        std::unique_ptr<VirtualSimulation> m_Simulation;
    };
}
