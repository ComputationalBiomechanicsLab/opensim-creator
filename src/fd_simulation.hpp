#pragma once

#include "opensim_wrapper.hpp"

namespace osmv {
    struct Fd_simulation_params final {
        Model model;

        // an initial state for the model
        //
        // if none is provided, the implementation will get the model's default
        // initial state
        State initial_state;

        // final time for the simulation. Assumed to be seconds
        double final_time = 10.0;

        // true if the simulation should slow down whenever it runs faster
        // than wall-time
        bool throttle_to_wall_time = true;

        Fd_simulation_params(osmv::Model m, osmv::State s, double t) :
            model{std::move(m)},
            initial_state{std::move(s)},
            final_time{t}
        {
        }
    };

    struct Fd_simulation_impl;
    class Fd_simulation final {
        std::unique_ptr<Fd_simulation_impl> impl;

    public:
        using clock = std::chrono::steady_clock;

        Fd_simulation(Fd_simulation_params);
        Fd_simulation(Fd_simulation const&) = delete;
        Fd_simulation(Fd_simulation&&) = delete;
        Fd_simulation& operator=(Fd_simulation const&) = delete;
        Fd_simulation& operator=(Fd_simulation&&) = delete;
        ~Fd_simulation() noexcept;  // cancels the simulation + cleans up

        // returns `true` if the simulator has a new latest state, with `dest`
        // being updated to match the latest state
        bool try_pop_latest_state(osmv::State& dest);

        // *requests* the simulator stop. The simulator may still be running after
        // returning from this function
        void request_stop();

        bool is_running() const;
        clock::duration wall_duration() const;
        double sim_current_time() const;
        double sim_final_time() const;
        char const* status_description() const;
        int num_prescribeq_calls() const;
        double avg_ui_overhead() const;
        int num_states_popped() const;
    };

	// just run a forward dynamics sim with default settings
	osmv::State run_fd_simulation(OpenSim::Model& model);
}
