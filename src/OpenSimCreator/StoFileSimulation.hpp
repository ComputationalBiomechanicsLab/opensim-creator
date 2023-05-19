#pragma once

#include "OpenSimCreator/SimulationClock.hpp"
#include "OpenSimCreator/SimulationReport.hpp"
#include "OpenSimCreator/SimulationStatus.hpp"
#include "OpenSimCreator/VirtualSimulation.hpp"

#include <oscar/Utils/SynchronizedValue.hpp>

#include <nonstd/span.hpp>

#include <filesystem>
#include <memory>
#include <vector>

namespace OpenSim { class Model; }
namespace osc { class OutputExtractor; }
namespace osc { class ParamBlock; }

namespace osc
{
    // an `osc::VirtualSimulation` that is directly loaded from an `.sto` file (as
    // opposed to being an actual simulation ran within `osc`)
    class StoFileSimulation final : public VirtualSimulation {
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

        int implGetNumReports() const final;
        SimulationReport implGetSimulationReport(int reportIndex) const final;
        std::vector<SimulationReport> implGetAllSimulationReports() const final;

        SimulationStatus implGetStatus() const final;
        SimulationClock::time_point implGetCurTime() const final;
        SimulationClock::time_point implGetStartTime() const final;
        SimulationClock::time_point implGetEndTime() const final;
        float implGetProgress() const final;
        ParamBlock const& implGetParams() const final;
        nonstd::span<OutputExtractor const> implGetOutputExtractors() const final;

        void implRequestStop() final;
        void implStop() final;

        float implGetFixupScaleFactor() const final;
        void implSetFixupScaleFactor(float) final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
