#pragma once

#include <libopensimcreator/documents/model/basic_model_state_pair.h>
#include <libopensimcreator/documents/simulation/abstract_simulation.h>
#include <libopensimcreator/documents/simulation/simulation_clock.h>
#include <libopensimcreator/documents/simulation/simulation_report.h>
#include <libopensimcreator/documents/simulation/simulation_status.h>

#include <liboscar/utilities/synchronized_value_guard.h>

#include <cstddef>
#include <memory>
#include <span>
#include <vector>

namespace opyn { class SharedOutputExtractor; }
namespace OpenSim { class Model; }
namespace osc { struct ForwardDynamicSimulatorParams; }
namespace osc { class ParamBlock; }

namespace osc
{
    // an `AbstractSimulation` that represents a live forward-dynamic simulation
    // that `osc` is running
    class ForwardDynamicSimulation final : public AbstractSimulation {
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
        std::span<const opyn::SharedOutputExtractor> implGetOutputExtractors() const final;

        bool implCanChangeEndTime() const final { return true; }
        void implRequestNewEndTime(SimulationClock::time_point) final;

        void implRequestStop() final;
        void implStop() final;

        float implGetFixupScaleFactor() const final;
        void implSetFixupScaleFactor(float) final;

        std::shared_ptr<Environment> implUpdAssociatedEnvironment() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
