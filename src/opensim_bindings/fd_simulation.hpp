#pragma once

#include <SimTKcommon.h>

#include <chrono>
#include <memory>

namespace OpenSim {
    class Model;
}

namespace osc::fd {

    // available integration methods
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

    // simulation parameters
    struct Params final {

        // final time for the simulation
        std::chrono::duration<double> final_time{1.0};

        // true if the simulation should slow down whenever it runs faster than wall-time
        bool throttle_to_wall_time = true;

        // which integration method to use for the simulation
        IntegratorMethod integrator_method = IntegratorMethod_OpenSimManagerDefault;

        // the time interval between report updates
        //
        // defaults to 60 FPS
        std::chrono::duration<double> reporting_interval{1.0/120.0};

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
    };

    // simulation input
    struct Input final {
        std::unique_ptr<OpenSim::Model> model;
        std::unique_ptr<SimTK::State> state;
        Params params;

        Input(std::unique_ptr<OpenSim::Model> _model, std::unique_ptr<SimTK::State> _state) :
            model{std::move(_model)}, state{std::move(_state)} {
        }
    };

    // stats collected whenever the simulation updates/reports
    struct Stats final {
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

    // report produced whenever
    //
    // - the "latest state" is empty (for .try_pop_latest_report)
    // - the next reporting interval is hit (for .pop_regular_reports)
    struct Report final {
        SimTK::State state;
        Stats stats;
    };

    // fd simulation that immediately starts running on a background thread
    class Simulation final {
        struct Impl;
        std::unique_ptr<Impl> impl;

    public:
        // starts the simulation on construction
        Simulation(std::unique_ptr<Input>);

        Simulation(Simulation const&) = delete;
        Simulation(Simulation&&) noexcept;

        // automatically cancels + joins the simulation thread
        //
        // rougly equivalent to calling .stop() on the simulator
        ~Simulation() noexcept;

        Simulation& operator=(Simulation const&) = delete;
        Simulation& operator=(Simulation&&) noexcept;

        // tries to pop the latest report from the simulator
        //
        // returns std::nullopt if the simulator thread hasn't populated a report
        // yet (i.e. if an integration/reporting step hasn't happened since the
        // last call)
        [[nodiscard]] std::unique_ptr<Report> try_pop_latest_report();
        [[nodiscard]] int num_latest_reports_popped() const noexcept;

        // these values are accurate to within one report, or integration step
        // (because the backend can only update them that often)

        [[nodiscard]] bool is_running() const noexcept;
        [[nodiscard]] std::chrono::duration<double> wall_duration() const noexcept;
        [[nodiscard]] std::chrono::duration<double> sim_current_time() const noexcept;
        [[nodiscard]] std::chrono::duration<double> sim_final_time() const noexcept;
        [[nodiscard]] char const* status_description() const noexcept;

        // progress of simulation, which falls in the range [0.0, 1.0]
        [[nodiscard]] float progress() const noexcept;

        // pushes regular reports onto the end of the outparam and returns the number
        // of reports popped
        //
        // - "regular reports" means the reports that are collected during the sim
        //   at `params.reporting_interval` intervals
        //
        // - this only pops the number of reports that the simulator has collected
        //   up to now. It may pop zero reports (e.g. if the caller pops more
        //   frequently than the simulator can report)
        //
        // - the sequence of reports, if all reports are popped, should be:
        //
        //       t0
        //       t0 + params.reporting_interval
        //       t0 + 2*params.reporting_interval
        //       ... t0 + n*params.reporting_interval ...
        //       tfinal (always reported - even if it is not a regular part of the sequence)
        int pop_regular_reports(std::vector<std::unique_ptr<Report>>& append_out);

        // requests that the simulator stops
        //
        // this is only a request: the simulation may still be running some time after
        // this method returns
        void request_stop() noexcept;

        // synchronously stop the simulation
        //
        // this method blocks until the simulation thread stops completely
        void stop() noexcept;
    };
}
