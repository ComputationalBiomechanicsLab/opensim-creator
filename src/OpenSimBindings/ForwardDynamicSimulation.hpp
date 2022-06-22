#pragma once

#include "src/OpenSimBindings/BasicModelStatePair.hpp"
#include "src/OpenSimBindings/SimulationClock.hpp"
#include "src/OpenSimBindings/SimulationReport.hpp"
#include "src/OpenSimBindings/SimulationStatus.hpp"
#include "src/OpenSimBindings/VirtualSimulation.hpp"
#include "src/Utils/SynchronizedValue.hpp"

#include <nonstd/span.hpp>

#include <vector>

namespace OpenSim
{
    class Model;
}

namespace osc
{
    struct ForwardDynamicSimulatorParams;
    class ParamBlock;
    class OutputExtractor;
}

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

        SynchronizedValueGuard<OpenSim::Model const> getModel() const override;

        int getNumReports() const override;
        SimulationReport getSimulationReport(int reportIndex) const override;
        std::vector<SimulationReport> getAllSimulationReports() const override;

        SimulationStatus getStatus() const override;
        SimulationClock::time_point getCurTime() const override;
        SimulationClock::time_point getStartTime() const override;
        SimulationClock::time_point getEndTime() const override;
        float getProgress() const override;
        ParamBlock const& getParams() const override;
        nonstd::span<OutputExtractor const> getOutputExtractors() const override;

        void requestStop() override;
        void stop() override;

        float getFixupScaleFactor() const override;
        void setFixupScaleFactor(float) override;

    private:
        class Impl;
        Impl* m_Impl;
    };
}
