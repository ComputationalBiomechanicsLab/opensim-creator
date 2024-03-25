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
            std::filesystem::path const& stoFilePath,
            float fixupScaleFactor
        );
        StoFileSimulation(StoFileSimulation const&) = delete;
        StoFileSimulation(StoFileSimulation&&) noexcept;
        StoFileSimulation& operator=(StoFileSimulation const&) = delete;
        StoFileSimulation& operator=(StoFileSimulation&&) noexcept;
        ~StoFileSimulation() noexcept;

    private:
        SynchronizedValueGuard<OpenSim::Model const> implGetModel() const final;

        ptrdiff_t implGetNumReports() const final;
        SimulationReport implGetSimulationReport(ptrdiff_t) const final;
        std::vector<SimulationReport> implGetAllSimulationReports() const final;

        SimulationStatus implGetStatus() const final;
        SimulationClocks implGetClocks() const final;
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
