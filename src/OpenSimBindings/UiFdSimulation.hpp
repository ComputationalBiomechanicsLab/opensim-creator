#pragma once

#include "src/OpenSimBindings/BasicModelStatePair.hpp"
#include "src/OpenSimBindings/SimulationClock.hpp"
#include "src/OpenSimBindings/SimulationReport.hpp"
#include "src/OpenSimBindings/SimulationStatus.hpp"
#include "src/OpenSimBindings/VirtualSimulation.hpp"

#include <nonstd/span.hpp>

#include <optional>
#include <memory>
#include <vector>

namespace OpenSim
{
    class Model;
    class Component;
}

namespace osc
{
    struct FdParams;
    class ParamBlock;
    class OutputExtractor;
    class SimulationReport;
}

namespace SimTK
{
    class State;
}

namespace osc
{
    // a simulation that represents a live forward-dynamic simulation
    class UiFdSimulation final : public VirtualSimulation {
    public:
        UiFdSimulation(BasicModelStatePair, FdParams const&);
        UiFdSimulation(UiFdSimulation const&) = delete;
        UiFdSimulation(UiFdSimulation&&) noexcept;
        UiFdSimulation& operator=(UiFdSimulation const&) = delete;
        UiFdSimulation& operator=(UiFdSimulation&&) noexcept;
        ~UiFdSimulation() noexcept;

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

        class Impl;
    private:
        std::unique_ptr<Impl> m_Impl;
    };
}
