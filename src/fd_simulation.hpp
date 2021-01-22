#pragma once

#include "opensim_wrapper.hpp"

namespace SimTK {
    class Integrator;
}

namespace osmv {
    // input parameters for a forward-dynamic simulation
    struct Fd_simulation_params final {
        Model model;

        // initial state of the model when the simulation starts
        State initial_state;

        // final time for the simulation. Assumed to be seconds
        double final_time = 10.0;

        // true if the simulation should slow down whenever it runs faster than wall-time
        bool throttle_to_wall_time = true;

        Fd_simulation_params(osmv::Model m, osmv::State s, double t) :
            model{std::move(m)},
            initial_state{std::move(s)},
            final_time{t} {
        }
    };

    // ongoing stats for the integrator that is running the fd simulation
    struct Integrator_stats final {
        float accuracyInUse = 0.0f;
        float predictedNextStepSize = 0.0f;

        int numStepsAttempted = 0;
        int numStepsTaken = 0;
        int numRealizations = 0;
        int numQProjections = 0;
        int numUProjections = 0;
        int numErrorTestFailures = 0;
        int numConvergenceTestFailures = 0;
        int numRealizationFailures = 0;
        int numQProjectionFailures = 0;
        int numUProjectionFailures = 0;
        int numProjectionFailures = 0;
        int numConvergentIterations = 0;
        int numDivergentIterations = 0;
        int numIterations = 0;

        Integrator_stats& operator=(SimTK::Integrator const&);
    };

    struct Fd_simulator_impl;

    // a simulator that runs a forward-dynamic simulation on a background thread
    class Fd_simulator final {
        std::unique_ptr<Fd_simulator_impl> impl;

    public:
        Fd_simulator(Fd_simulation_params);
        Fd_simulator(Fd_simulator const&) = delete;
        Fd_simulator(Fd_simulator&&) = delete;
        Fd_simulator& operator=(Fd_simulator const&) = delete;
        Fd_simulator& operator=(Fd_simulator&&) = delete;

        // destruction of a simulator:
        //
        // - cancels the simulation
        // - waits for the simulation to stop
        // - joins the simulation thread
        // - cleans up anything internal
        ~Fd_simulator() noexcept;

        // returns `true` if a state was written into `dest`
        //
        // internally, the simulator's state is copied into a "message space" that the simulator
        // thread will fill whenever it sees that the space is empty. Therefore, the state that
        // is popped is *not* the latest state, but is effectively the "first state after the
        // last pop"
        //
        // the reason why it isn't guaranteed to be the latest state is an optimization: the
        // simulator thread only has to do extra work if some other thread is continually popping
        // states off of it.
        bool try_pop_state(osmv::State& dest);
        int num_states_popped() const;

        // requests the simulator stop
        //
        // - this is only a *request*: the simulation may still be running after this method
        //   returns because it may take a nonzero amount of time to propagate the request
        void request_stop();

        // stop the simulation
        //
        // this method waits until the simulation stops
        void stop();

        // below: these getters reflect the latest state of the simulation, accurate
        // to within one integration step (unlike state popping, which has the listed
        // caveats)

        bool is_running() const;
        std::chrono::duration<double> wall_duration() const;
        std::chrono::duration<double> sim_current_time() const;
        std::chrono::duration<double> sim_final_time() const;
        char const* status_description() const;
        int num_prescribeq_calls() const;
        Integrator_stats integrator_stats() const noexcept;

        // an estimate of what percentage of time the simulator thread is spending
        // upholding the simulator's requirements (e.g. copying states, reporting
        // stats)
        double avg_simulator_overhead() const;
    };

    // run a forward-dynamic simulation on the current thread
    osmv::State run_fd_simulation(OpenSim::Model& model);
}
