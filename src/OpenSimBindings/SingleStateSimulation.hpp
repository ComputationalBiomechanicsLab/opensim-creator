#pragma once

#include "src/OpenSimBindings/BasicModelStatePair.hpp"
#include "src/OpenSimBindings/SimulationClock.hpp"
#include "src/OpenSimBindings/SimulationReport.hpp"
#include "src/OpenSimBindings/SimulationStatus.hpp"
#include "src/OpenSimBindings/VirtualSimulation.hpp"
#include "src/Utils/SynchronizedValue.hpp"

#include <nonstd/span.hpp>

#include <memory>
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

        SynchronizedValueGuard<OpenSim::Model const> getModel() const final;

        int getNumReports() const final;
        SimulationReport getSimulationReport(int reportIndex) const final;
        std::vector<SimulationReport> getAllSimulationReports() const final;

        SimulationStatus getStatus() const final;
        SimulationClock::time_point getCurTime() const final;
        SimulationClock::time_point getStartTime() const final;
        SimulationClock::time_point getEndTime() const final;
        float getProgress() const final;
        ParamBlock const& getParams() const final;
        nonstd::span<OutputExtractor const> getOutputExtractors() const final;

        void requestStop() final;
        void stop() final;

        float getFixupScaleFactor() const final;
        void setFixupScaleFactor(float) final;

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}