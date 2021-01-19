#pragma once

#include "opensim_wrapper.hpp"

namespace osmv {
    // parameters for a forward-dynamic simulation
    struct Fd_simulation_params final {
        Model model;

        // initial state of the model when the simulation starts
        State initial_state;

        // final time for the simulation. Assumed to be seconds
        double final_time = 10.0;

        // true if the simulation should slow down whenever it runs faster
        // than wall-time
        bool throttle_to_wall_time = true;

        Fd_simulation_params(osmv::Model m, osmv::State s, double t) :
            model{std::move(m)},
            initial_state{std::move(s)},
            final_time{t} {
        }
    };

    struct Fd_simulator_impl;
    class Fd_simulator final {
        std::unique_ptr<Fd_simulator_impl> impl;

    public:
        Fd_simulator(Fd_simulation_params);
        Fd_simulator(Fd_simulator const&) = delete;
        Fd_simulator(Fd_simulator&&) = delete;
        Fd_simulator& operator=(Fd_simulator const&) = delete;
        Fd_simulator& operator=(Fd_simulator&&) = delete;
        ~Fd_simulator() noexcept;  // cancels the simulation + cleans up

        // returns `true` if the simulator has a new latest state, with `dest`
        // being updated to match the latest state
        bool try_pop_latest_state(osmv::State& dest);

        // *requests* the simulator stop. The simulator may still be running after
        // returning from this function
        void request_stop();

        bool is_running() const;
        std::chrono::duration<double> wall_duration() const;
        std::chrono::duration<double> sim_current_time() const;
        std::chrono::duration<double> sim_final_time() const;
        char const* status_description() const;
        int num_prescribeq_calls() const;
        double avg_ui_overhead_pct() const;
        int num_states_popped() const;
        int num_integration_steps() const;
        int num_integration_step_attempts() const;
    };

    // run a forward-dynamic simulation on the current thread
    osmv::State run_fd_simulation(OpenSim::Model& model);
}
