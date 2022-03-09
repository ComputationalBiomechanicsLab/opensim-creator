#pragma once

#include "src/OpenSimBindings/SimulationClock.hpp"
#include "src/OpenSimBindings/SimulationStatus.hpp"
#include "src/OpenSimBindings/VirtualSimulation.hpp"
#include "src/OpenSimBindings/SimulationReport.hpp"
#include "src/Utils/UID.hpp"

#include <nonstd/span.hpp>

#include <memory>
#include <optional>
#include <vector>
#include <string>

namespace osc
{
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
    public:
        template<class ConcreteSimulation>
        Simulation(ConcreteSimulation&& simulation) :
            m_Simulation{std::make_unique<ConcreteSimulation>(std::forward<ConcreteSimulation>(simulation))}
        {
        }

        OpenSim::Model const& getModel() const { return m_Simulation->getModel(); }

        int getNumReports() { return m_Simulation->getNumReports(); }
        SimulationReport getSimulationReport(int reportIndex) { return m_Simulation->getSimulationReport(std::move(reportIndex)); }
        std::vector<SimulationReport> getAllSimulationReports() const { return m_Simulation->getAllSimulationReports(); }

        SimulationStatus getStatus() const { return m_Simulation->getStatus(); }
        osc::SimulationClock::time_point getCurTime() { return m_Simulation->getCurTime(); }
        osc::SimulationClock::time_point getStartTime() const { return m_Simulation->getStartTime(); }
        osc::SimulationClock::time_point getEndTime() const { return m_Simulation->getEndTime(); }
        float getProgress() const { return m_Simulation->getProgress(); }
        ParamBlock const& getParams() const { return m_Simulation->getParams(); }
        nonstd::span<Output const> getOutputs() const { return m_Simulation->getOutputs(); }

        void requestStop() { m_Simulation->requestStop(); }
        void stop() { m_Simulation->stop(); }

        operator VirtualSimulation& () { return *m_Simulation; }
        operator VirtualSimulation const& () const { return *m_Simulation; }

    private:
        std::unique_ptr<VirtualSimulation> m_Simulation;
    };
}
