#pragma once

#include <libopensimcreator/documents/simulation/abstract_simulation.h>
#include <libopensimcreator/documents/simulation/simulation_clocks.h>
#include <libopensimcreator/documents/simulation/simulation_report.h>
#include <libopensimcreator/documents/simulation/simulation_status.h>

#include <liboscar/utilities/synchronized_value_guard.h>

#include <cstddef>
#include <filesystem>
#include <memory>
#include <span>
#include <vector>

namespace opyn { class SharedOutputExtractor; }
namespace OpenSim { class Model; }
namespace osc { class Environment; }
namespace osc { class ParamBlock; }

namespace osc
{
    // an `AbstractSimulation` that is directly loaded from an `.sto` file (as
    // opposed to being an actual simulation ran within `osc`)
    class StoFileSimulation final : public AbstractSimulation {
    public:
        StoFileSimulation(
            std::unique_ptr<OpenSim::Model>,
            const std::filesystem::path& stoFilePath,
            float fixupScaleFactor,
            std::shared_ptr<Environment>
        );
        StoFileSimulation(const StoFileSimulation&) = delete;
        StoFileSimulation(StoFileSimulation&&) noexcept;
        StoFileSimulation& operator=(const StoFileSimulation&) = delete;
        StoFileSimulation& operator=(StoFileSimulation&&) noexcept;
        ~StoFileSimulation() noexcept;

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
