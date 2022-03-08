#pragma once

#include "src/OpenSimBindings/BasicModelStatePair.hpp"
#include "src/OpenSimBindings/SimulationClock.hpp"
#include "src/OpenSimBindings/SimulationStatus.hpp"
#include "src/OpenSimBindings/VirtualSimulation.hpp"
#include "src/Utils/UID.hpp"

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
    class Output;
    class SimulationReport;
}

namespace SimTK
{
    class State;
}

namespace osc
{
    // maintains a forward-dynamic simulation
    class UiFdSimulation final : public VirtualSimulation {
    public:
        UiFdSimulation(BasicModelStatePair, FdParams const&);
        UiFdSimulation(UiFdSimulation const&) = delete;
        UiFdSimulation(UiFdSimulation&&) noexcept;
        UiFdSimulation& operator=(UiFdSimulation const&) = delete;
        UiFdSimulation& operator=(UiFdSimulation&&) noexcept;
        ~UiFdSimulation() noexcept;

        OpenSim::Model const& getModel() const override;

        int getNumReports() override;
        SimulationReport getSimulationReport(int reportIndex) override;

        SimulationStatus getSimulationStatus() const override;
        void requestStop() override;
        void stop() override;
        SimulationClock::time_point getSimulationCurTime() override;
        SimulationClock::time_point getSimulationStartTime() const override;
        SimulationClock::time_point getSimulationEndTime() const override;
        ParamBlock const& getSimulationParams() const override;
        nonstd::span<Output const> getOutputs() const override;

        class Impl;
    private:
        std::unique_ptr<Impl> m_Impl;
    };
}
