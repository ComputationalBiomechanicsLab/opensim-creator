#pragma once

#include <OpenSimCreator/Documents/Simulation/ISimulation.h>
#include <OpenSimCreator/Documents/Simulation/SimulationClock.h>
#include <OpenSimCreator/Documents/Simulation/SimulationReport.h>
#include <OpenSimCreator/Documents/Simulation/SimulationStatus.h>

#include <oscar/Utils/SynchronizedValueGuard.h>

#include <cstddef>
#include <filesystem>
#include <memory>
#include <span>
#include <vector>

namespace OpenSim { class Model; }
namespace osc { class Environment; }
namespace osc { class OutputExtractor; }
namespace osc { class ParamBlock; }

namespace osc
{
    // an `ISimulation` that is directly loaded from an `.sto` file (as
    // opposed to being an actual simulation ran within `osc`)
    class StoFileSimulation final : public ISimulation {
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
        std::span<const OutputExtractor> implGetOutputExtractors() const final;

        float implGetFixupScaleFactor() const final;
        void implSetFixupScaleFactor(float) final;

        std::shared_ptr<Environment> implUpdAssociatedEnvironment() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
