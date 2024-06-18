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

namespace osc
{
    class SingleStateSimulation final : public ISimulation {
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
        std::span<const OutputExtractor> implGetOutputExtractors() const final;

        float implGetFixupScaleFactor() const final;
        void implSetFixupScaleFactor(float) final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
