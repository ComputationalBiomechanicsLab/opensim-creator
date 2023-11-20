#pragma once

#include <OpenSimCreator/Model/BasicModelStatePair.hpp>
#include <OpenSimCreator/Simulation/SimulationClock.hpp>
#include <OpenSimCreator/Simulation/SimulationReport.hpp>
#include <OpenSimCreator/Simulation/SimulationStatus.hpp>
#include <OpenSimCreator/Simulation/VirtualSimulation.hpp>

#include <oscar/Utils/SynchronizedValueGuard.hpp>

#include <cstddef>
#include <memory>
#include <span>
#include <vector>

namespace OpenSim { class Model; }

namespace osc
{
    class SingleStateSimulation final : public VirtualSimulation {
    public:
        explicit SingleStateSimulation(BasicModelStatePair);
        SingleStateSimulation(SingleStateSimulation const&) = delete;
        SingleStateSimulation(SingleStateSimulation&&) noexcept;
        SingleStateSimulation& operator=(SingleStateSimulation const&) = delete;
        SingleStateSimulation& operator=(SingleStateSimulation&&) noexcept;
        ~SingleStateSimulation() noexcept;

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
