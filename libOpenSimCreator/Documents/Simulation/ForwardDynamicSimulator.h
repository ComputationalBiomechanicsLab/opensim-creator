#pragma once

#include <libOpenSimCreator/Documents/Model/BasicModelStatePair.h>
#include <libOpenSimCreator/Documents/OutputExtractors/OutputExtractor.h>
#include <libOpenSimCreator/Documents/Simulation/SimulationStatus.h>

#include <functional>
#include <memory>

namespace osc { struct ForwardDynamicSimulatorParams; }
namespace osc { class SimulationReport; }

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
        ForwardDynamicSimulator(
            BasicModelStatePair,
            const ForwardDynamicSimulatorParams&,
            std::function<void(SimulationReport)> onReportFromBgThread
        );
        ForwardDynamicSimulator(const ForwardDynamicSimulator&) = delete;
        ForwardDynamicSimulator(ForwardDynamicSimulator&&) noexcept;
        ForwardDynamicSimulator& operator=(const ForwardDynamicSimulator&) = delete;
        ForwardDynamicSimulator& operator=(ForwardDynamicSimulator&&) noexcept;
        ~ForwardDynamicSimulator() noexcept;

        SimulationStatus getStatus() const;

        // blocks the current thread until the simulator thread finishes its execution
        void join();

        // asynchronous
        void requestStop();

        // synchronous (blocks until it stops)
        void stop();

        const ForwardDynamicSimulatorParams& params() const;

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
