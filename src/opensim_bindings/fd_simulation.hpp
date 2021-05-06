#pragma once

#include <chrono>
#include <memory>

namespace SimTK {
    class State;
}

namespace OpenSim {
    class Model;
}

namespace osc {

    enum IntegratorMethod {
        IntegratorMethod_OpenSimManagerDefault = 0,
        IntegratorMethod_ExplicitEuler,
        IntegratorMethod_RungeKutta2,
        IntegratorMethod_RungeKutta3,
        IntegratorMethod_RungeKuttaFeldberg,
        IntegratorMethod_RungeKuttaMerson,
        IntegratorMethod_SemiExplicitEuler2,
        IntegratorMethod_Verlet,

        IntegratorMethod_NumIntegratorMethods,
    };
    extern IntegratorMethod const integrator_methods[IntegratorMethod_NumIntegratorMethods];
    extern char const* const integrator_method_names[IntegratorMethod_NumIntegratorMethods];

    // input parameters for a forward-dynamic simulation
    struct Fd_simulation_params final {
        // model to simulate
        std::unique_ptr<OpenSim::Model> model;

        // initial state when the simulation starts
        std::unique_ptr<SimTK::State> state;

        // final time for the simulation
        std::chrono::duration<double> final_time{10.0};

        // true if the simulation should slow down whenever it runs faster than wall-time
        bool throttle_to_wall_time = true;

        // which integration method to use for the simulation
        IntegratorMethod integrator_method = IntegratorMethod_OpenSimManagerDefault;

        // the time interval between report updates
        //
        // defaults to 60 FPS
        std::chrono::duration<double> reporting_interval{1.0/60.0};

        // max number of *internal* steps that may be taken within a single call
        // to the integrator's stepTo or stepBy function
        //
        // this is mostly an internal concern, but can affect how regularly the
        // simulator reports updates (e.g. a lower number here *may* mean more
        // frequent per-significant-step updates)
        int integrator_step_limit = 20000;

        // minimum step, in time, that the integrator should attempt
        //
        // some integrators just ignore this
        std::chrono::duration<double> integrator_minimum_step_size{1.0e-8};

        // maximum step, in time, that an integrator can attempt
        //
        // e.g. even if the integrator *thinks* it can skip 10s of simulation time
        // it still *must* integrate to this size and return to the caller (i.e. the
        // simulator) to report the state at this maximum time
        std::chrono::duration<double> integrator_maximum_step_size{1.0};

        // accuracy of the integrator
        //
        // this only does something if the integrator is error-controlled and able
        // to improve accuracy (e.g. by taking many more steps)
        double integrator_accuracy = 1.0e-5;

        // set whether the latest state update from the simulator should be posted
        // on every step (if not yet popped)
        //
        // else: the update is only posted whenever the regular reporting interval
        // is set
        bool update_latest_state_on_every_step = true;

        Fd_simulation_params(
            OpenSim::Model const& _model,
            SimTK::State const& _state,
            std::chrono::duration<double> _final_time,
            IntegratorMethod _method) :

            model{std::make_unique<OpenSim::Model>(_model)},
            state{std::make_unique<SimTK::State>(_state)},
            final_time{_final_time},
            integrator_method{_method} {
        }
    };

    // aggregated stats for a simulation
    struct Simulation_stats final {
        // top-level stats
        std::chrono::duration<double> time;
        float ui_overhead_estimated_ratio = 0.0f;

        // integrator stats
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

        // system stats
        int numPrescribeQcalls = 0;
    };

    // a simulator that runs a forward-dynamic simulation on a background thread
    class Fd_simulation final {
        struct Impl;
        std::unique_ptr<Impl> impl;

    public:
        // starts the simulation on construction
        Fd_simulation(Fd_simulation_params);

        // automatically cancels + joins the simulation thread
        ~Fd_simulation() noexcept;

        // returns the latest state available state, populated by the sim thread, or
        // `nullptr` if no state is available.
        //
        // internally, the simulator's state is copied into a "message space" that the simulator
        // thread will fill whenever it sees that the space is empty. Therefore, the state that
        // is popped is *not* the latest state, but is effectively the "the first state after the
        // last successful pop"
        //
        // the reason why it isn't guaranteed to be the latest state is an optimization: the
        // simulator thread only has to do extra work if some other thread is continually popping
        // states off of it. So if you do not call this, then it has (almost) no overhead.
        [[nodiscard]] std::unique_ptr<SimTK::State> try_pop_state();
        [[nodiscard]] int num_states_popped() const noexcept;

        // below: these getters reflect the latest state of the simulation, accurate
        // to within one integration step (unlike state popping, which has the listed
        // caveats)

        [[nodiscard]] bool is_running() const noexcept;
        [[nodiscard]] std::chrono::duration<double> wall_duration() const noexcept;
        [[nodiscard]] std::chrono::duration<double> sim_current_time() const noexcept;
        [[nodiscard]] std::chrono::duration<double> sim_final_time() const noexcept;
        [[nodiscard]] char const* status_description() const noexcept;
        [[nodiscard]] Simulation_stats stats() const noexcept;

        // requests that the simulator stop
        //
        // - this is only a *request*: the simulation may still be running after this method
        //   returns because it may take a nonzero amount of time to propagate the request
        void request_stop() noexcept;

        // stop the simulation
        //
        // this method waits until the simulation stops
        void stop() noexcept;
    };
}
