#pragma once

#include <OpenSimCreator/Documents/Model/BasicModelStatePair.h>
#include <OpenSimCreator/Documents/Simulation/ISimulation.h>
#include <OpenSimCreator/Documents/Simulation/SimulationClock.h>
#include <OpenSimCreator/Documents/Simulation/SimulationReport.h>
#include <OpenSimCreator/Documents/Simulation/SimulationStatus.h>

#include <oscar/Utils/SynchronizedValueGuard.h>

#include <cstddef>
#include <memory>
#include <span>
#include <vector>

namespace OpenSim { class Model; }
namespace osc { struct ForwardDynamicSimulatorParams; }
namespace osc { class ParamBlock; }
namespace osc { class OutputExtractor; }

namespace osc
{
    // an `ISimulation` that represents a live forward-dynamic simulation
    // that `osc` is running
    class ForwardDynamicSimulation final : public ISimulation {
    public:
        ForwardDynamicSimulation(BasicModelStatePair, const ForwardDynamicSimulatorParams&);
        ForwardDynamicSimulation(const ForwardDynamicSimulation&) = delete;
        ForwardDynamicSimulation(ForwardDynamicSimulation&&) noexcept;
        ForwardDynamicSimulation& operator=(const ForwardDynamicSimulation&) = delete;
        ForwardDynamicSimulation& operator=(ForwardDynamicSimulation&&) noexcept;
        ~ForwardDynamicSimulation() noexcept;

        // blocks the current thread until the simulator thread finishes its execution
        void join();
    private:
        SynchronizedValueGuard<const OpenSim::Model> implGetModel() const final;

        ptrdiff_t implGetNumReports() const final;
        SimulationReport implGetSimulationReport(ptrdiff_t) const final;
        std::vector<SimulationReport> implGetAllSimulationReports() const final;

        SimulationStatus implGetStatus() const final;
        SimulationClocks implGetClocks() const final;
        const ParamBlock& implGetParams() const final;
        std::span<const OutputExtractor> implGetOutputExtractors() const final;

        bool implCanChangeEndTime() const final { return true; }
        void implRequestNewEndTime(SimulationClock::time_point) final;

        void implRequestStop() final;
        void implStop() final;

        float implGetFixupScaleFactor() const final;
        void implSetFixupScaleFactor(float) final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
