#pragma once

#include <OpenSimCreator/Simulation/SimulationClock.hpp>
#include <OpenSimCreator/Simulation/SimulationStatus.hpp>
#include <OpenSimCreator/Simulation/VirtualSimulation.hpp>
#include <OpenSimCreator/Simulation/SimulationReport.hpp>

#include <oscar/Utils/SynchronizedValue.hpp>
#include <nonstd/span.hpp>

#include <cstddef>
#include <memory>
#include <vector>

namespace osc { class OutputExtractor; }
namespace osc { class ParamBlock; }
namespace OpenSim { class Model; }
namespace SimTK { class State; }

namespace osc
{
    // a concrete value type wrapper for an `osc::VirtualSimulation`
    //
    // This is a value-type that can be compared, hashed, etc. for easier usage
    // by other parts of osc (e.g. aggregators, plotters)
    class Simulation final {
    public:
        template<typename TConcreteSimulation>
        Simulation(TConcreteSimulation&& simulation) :
            m_Simulation{std::make_unique<TConcreteSimulation>(std::forward<TConcreteSimulation>(simulation))}
        {
        }

        SynchronizedValueGuard<OpenSim::Model const> getModel() const { return m_Simulation->getModel(); }

        size_t getNumReports() const { return m_Simulation->getNumReports(); }
        SimulationReport getSimulationReport(ptrdiff_t reportIndex) const { return m_Simulation->getSimulationReport(std::move(reportIndex)); }
        std::vector<SimulationReport> getAllSimulationReports() const { return m_Simulation->getAllSimulationReports(); }

        SimulationStatus getStatus() const { return m_Simulation->getStatus(); }
        SimulationClock::time_point getCurTime() { return m_Simulation->getCurTime(); }
        SimulationClock::time_point getStartTime() const { return m_Simulation->getStartTime(); }
        SimulationClock::time_point getEndTime() const { return m_Simulation->getEndTime(); }
        float getProgress() const { return m_Simulation->getProgress(); }
        ParamBlock const& getParams() const { return m_Simulation->getParams(); }
        nonstd::span<OutputExtractor const> getOutputs() const { return m_Simulation->getOutputExtractors(); }

        void requestStop() { m_Simulation->requestStop(); }
        void stop() { m_Simulation->stop(); }

        float getFixupScaleFactor() const { return m_Simulation->getFixupScaleFactor(); }
        void setFixupScaleFactor(float v) { m_Simulation->setFixupScaleFactor(v); }

        operator VirtualSimulation& () { return *m_Simulation; }
        operator VirtualSimulation const& () const { return *m_Simulation; }

    private:
        std::unique_ptr<VirtualSimulation> m_Simulation;
    };
}
