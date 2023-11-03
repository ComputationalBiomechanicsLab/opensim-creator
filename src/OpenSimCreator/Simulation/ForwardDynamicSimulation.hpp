#pragma once

#include <OpenSimCreator/Model/BasicModelStatePair.hpp>
#include <OpenSimCreator/Simulation/SimulationClock.hpp>
#include <OpenSimCreator/Simulation/SimulationReport.hpp>
#include <OpenSimCreator/Simulation/SimulationStatus.hpp>
#include <OpenSimCreator/Simulation/VirtualSimulation.hpp>

#include <oscar/Utils/SynchronizedValue.hpp>

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
    // an `osc::VirtualSimulation` that represents a live forward-dynamic simulation
    // that `osc` is running
    class ForwardDynamicSimulation final : public VirtualSimulation {
    public:
        ForwardDynamicSimulation(BasicModelStatePair, ForwardDynamicSimulatorParams const&);
        ForwardDynamicSimulation(ForwardDynamicSimulation const&) = delete;
        ForwardDynamicSimulation(ForwardDynamicSimulation&&) noexcept;
        ForwardDynamicSimulation& operator=(ForwardDynamicSimulation const&) = delete;
        ForwardDynamicSimulation& operator=(ForwardDynamicSimulation&&) noexcept;
        ~ForwardDynamicSimulation() noexcept;

    private:
        SynchronizedValueGuard<OpenSim::Model const> implGetModel() const final;

        ptrdiff_t implGetNumReports() const final;
        SimulationReport implGetSimulationReport(ptrdiff_t) const final;
        std::vector<SimulationReport> implGetAllSimulationReports() const final;

        SimulationStatus implGetStatus() const final;
        SimulationClock::time_point implGetCurTime() const final;
        SimulationClock::time_point implGetStartTime() const final;
        SimulationClock::time_point implGetEndTime() const final;
        float implGetProgress() const final;
        ParamBlock const& implGetParams() const final;
        std::span<OutputExtractor const> implGetOutputExtractors() const final;

        void implRequestStop() final;
        void implStop() final;

        float implGetFixupScaleFactor() const final;
        void implSetFixupScaleFactor(float) final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
