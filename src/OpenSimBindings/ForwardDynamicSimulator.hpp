#pragma once

#include "src/OpenSimBindings/BasicModelStatePair.hpp"
#include "src/OpenSimBindings/OutputExtractor.hpp"
#include "src/OpenSimBindings/SimulationStatus.hpp"

#include <functional>

namespace OpenSim
{
    class Model;
}

namespace osc
{
    struct ForwardDynamicSimulatorParams;
    class SimulationReport;
}

namespace osc
{
    // returns outputs (e.g. auxiliary stuff like integration steps) that the
    // FdSimulator writes into the `SimulationReport`s it emits
    int GetNumFdSimulatorOutputExtractors();
    OutputExtractor GetFdSimulatorOutputExtractor(int);

    // a forward-dynamic simulation that immediately starts running on a background thread
    class ForwardDynamicSimulator final {
    public:
        // immediately starts the simulation upon construction
        //
        // care: the callback is called *on the bg thread* - you should know how
        //       to handle it (e.g. mutexes) appropriately
        ForwardDynamicSimulator(BasicModelStatePair,
                     ForwardDynamicSimulatorParams const& params,
                     std::function<void(SimulationReport)> onReportFromBgThread);

        ForwardDynamicSimulator(ForwardDynamicSimulator const&) = delete;
        ForwardDynamicSimulator(ForwardDynamicSimulator&&) noexcept;
        ForwardDynamicSimulator& operator=(ForwardDynamicSimulator const&) = delete;
        ForwardDynamicSimulator& operator=(ForwardDynamicSimulator&&) noexcept;
        ~ForwardDynamicSimulator() noexcept;

        SimulationStatus getStatus() const;
        void requestStop();  // asynchronous
        void stop();  // synchronous (blocks until it stops)
        ForwardDynamicSimulatorParams const& params() const;

    private:
        class Impl;
        Impl* m_Impl;
    };
}
