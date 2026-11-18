#pragma once

#include <libopensimcreator/documents/model/basic_model_state_pair_with_shared_environment.h>
#include <libopensimcreator/documents/simulation/simulation_status.h>

#include <libopynsim/documents/output_extractors/shared_output_extractor.h>

#include <functional>
#include <memory>

namespace osc
{
    struct ForwardDynamicSimulatorParams;
    class SimulationReport;

    // returns outputs (e.g. auxiliary stuff like integration steps) that the
    // FdSimulator writes into the `SimulationReport`s it emits
    int get_num_fd_simulator_output_extractors();
    opyn::SharedOutputExtractor get_fd_simulator_output_extractor(int);

    // a forward-dynamic simulation that immediately starts running on a background thread
    class ForwardDynamicSimulator final {
    public:
        // immediately starts the simulation upon construction
        //
        // care: the callback is called *on the bg thread* - you should know how
        //       to handle it (e.g. mutexes) appropriately
        ForwardDynamicSimulator(
            BasicModelStatePairWithSharedEnvironment,
            const ForwardDynamicSimulatorParams&,
            std::function<void(SimulationReport)> on_report_from_bg_thread);
        ForwardDynamicSimulator(const ForwardDynamicSimulator&) = delete;
        ForwardDynamicSimulator(ForwardDynamicSimulator&&) noexcept;
        ForwardDynamicSimulator& operator=(const ForwardDynamicSimulator&) = delete;
        ForwardDynamicSimulator& operator=(ForwardDynamicSimulator&&) noexcept;
        ~ForwardDynamicSimulator() noexcept;

        SimulationStatus get_status() const;

        // blocks the current thread until the simulator thread finishes its execution
        void join();

        // asynchronous
        void request_stop();

        // synchronous (blocks until it stops)
        void stop();

        const ForwardDynamicSimulatorParams& params() const;

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}
