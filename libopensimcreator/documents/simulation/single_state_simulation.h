#pragma once

#include <libopensimcreator/documents/model/basic_model_state_pair.h>
#include <libopensimcreator/documents/simulation/abstract_simulation.h>
#include <libopensimcreator/documents/simulation/simulation_clocks.h>
#include <libopensimcreator/documents/simulation/simulation_report.h>
#include <libopensimcreator/documents/simulation/simulation_status.h>

#include <liboscar/utilities/synchronized_value_guard.h>

#include <cstddef>
#include <memory>
#include <span>
#include <vector>

namespace OpenSim { class Model; }
namespace osc { class Environment; }

namespace osc
{
    class SingleStateSimulation final : public AbstractSimulation {
    public:
        explicit SingleStateSimulation(BasicModelStatePair);
        SingleStateSimulation(const SingleStateSimulation&) = delete;
        SingleStateSimulation(SingleStateSimulation&&) noexcept;
        SingleStateSimulation& operator=(const SingleStateSimulation&) = delete;
        SingleStateSimulation& operator=(SingleStateSimulation&&) noexcept;
        ~SingleStateSimulation() noexcept;

    private:
        SynchronizedValueGuard<const OpenSim::Model> implGetModel() const final;

        ptrdiff_t implGetNumReports() const final;
        SimulationReport implGetSimulationReport(ptrdiff_t) const final;
        std::vector<SimulationReport> implGetAllSimulationReports() const final;

        SimulationStatus implGetStatus() const final;
        SimulationClocks implGetClocks() const final;
        const ParamBlock& implGetParams() const final;
        std::span<const opyn::SharedOutputExtractor> implGetOutputExtractors() const final;

        float implGetFixupScaleFactor() const final;
        void implSetFixupScaleFactor(float) final;

        std::shared_ptr<Environment> implUpdAssociatedEnvironment() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
