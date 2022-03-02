#pragma once

#include "src/OpenSimBindings/BasicModelStatePair.hpp"

#include <SimTKcommon.h>

#include <chrono>
#include <memory>
#include <vector>

namespace OpenSim {
    class Model;
}

namespace osc {

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
    extern IntegratorMethod const g_IntegratorMethods[IntegratorMethod_NumIntegratorMethods];
    extern char const* const g_IntegratorMethodNames[IntegratorMethod_NumIntegratorMethods];



    // simulation parameters
    struct FdParams final {

        // final time for the simulation
        std::chrono::duration<double> FinalTime{10.0};
        static constexpr char const* g_FinalTimeTitle = "final time (sec)";
        static constexpr char const* g_FinalTimeDesc = "The final time, in seconds, that the forward dynamic simulation should integrate up to";

        // true if the simulation should slow down whenever it runs faster than wall-time
        bool ThrottleToWallTime = true;
        static constexpr char const* g_ThrottleToWallTimeTitle = "throttle to wall time";
        static constexpr char const* g_ThrottleToWallTimeDesc = "Whether the simulator should slow down whenever it is running faster than real time. This is useful for visualizing the simulation 'as it runs' - especially when the simulation would complete much faster than the simulation time";

        // which integration method to use for the simulation
        IntegratorMethod IntegratorMethodUsed = IntegratorMethod_OpenSimManagerDefault;
        static constexpr char const* g_IntegratorMethodUsedTitle = "integrator method";
        static constexpr char const* g_IntegratorMethodUsedDesc = "The integrator that the forward dynamic simulator should use. OpenSim's default integrator is a good choice if you aren't familiar with the other integrators. Changing the integrator can have a large impact on the performance and accuracy of the simulation.";

        // the time interval, in simulation time, between report updates
        std::chrono::duration<double> ReportingInterval{1.0/120.0};
        static constexpr char const* g_ReportingIntervalTitle = "reporting interval";
        static constexpr char const* g_ReportingIntervalDesc = "How often the simulator should emit a simulation report. This affects how many datapoints are collected for the animation, output values, etc.";

        // max number of *internal* steps that may be taken within a single call
        // to the integrator's stepTo or stepBy function
        //
        // this is mostly an internal concern, but can affect how regularly the
        // simulator reports updates (e.g. a lower number here *may* mean more
        // frequent per-significant-step updates)
        int IntegratorStepLimit = 20000;
        static constexpr char const* g_IntegratorStepLimitTitle = "integrator step limit";
        static constexpr char const* g_IntegratorStepLimitDesc = "The maximum number of *internal* steps that can be taken within a single call to the integrator's stepTo/stepBy function. This is mostly an internal engine concern, but can occasionally affect how often reports are emitted";

        // minimum step, in time, that the integrator should attempt
        //
        // some integrators just ignore this
        std::chrono::duration<double> IntegratorMinimumStepSize{1.0e-8};
        static constexpr char const* g_IntegratorMinimumStepSizeTitle = "integrator minimum step size (sec)";
        static constexpr char const* g_IntegratorMinimumStepSizeDesc = "The minimum step size, in time, that the integrator must take during the simulation. Note: this is mostly only relevant for error-corrected integrators that change their step size dynamically as the simulation runs.";

        // maximum step, in time, that an integrator can attempt
        //
        // e.g. even if the integrator *thinks* it can skip 10s of simulation time
        // it still *must* integrate to this size and return to the caller (i.e. the
        // simulator) to report the state at this maximum time
        std::chrono::duration<double> IntegratorMaximumStepSize{1.0};
        static constexpr char const* g_IntegratorMaximumStepSizeTitle = "integrator maximum step size (sec)";
        static constexpr char const* g_IntegratorMaximumStepSizeDesc = "The maximum step size, in seconds, that the integrator must take during the simulation. Note: this is mostly only relevant for error-correct integrators that change their step size dynamically as the simulation runs";

        // accuracy of the integrator
        //
        // this only does something if the integrator is error-controlled and able
        // to improve accuracy (e.g. by taking many more steps)
        double IntegratorAccuracy = 1.0e-5;
        static constexpr char const* g_IntegratorAccuracyTitle = "integrator accuracy";
        static constexpr char const* g_IntegratorAccuracyDesc = "Target accuracy for the integrator. Mostly only relevant for error-controlled integrators that change their step size by comparing this accuracy value to measured integration error";

        // set whether the latest state update from the simulator should be posted
        // on every step (if not yet popped)
        //
        // else: the update is only posted whenever the regular reporting interval
        // is set
        bool UpdateLatestStateOnEveryStep = true;
        static constexpr char const* g_UpdateLatestStateOnEveryStepTitle = "update latest state on every step";
        static constexpr char const* g_UpdateLatestStateOnEveryStepDesc = "Whether the simulator should try to update the latest integration state on each integration step. Internally, the UI will frequently ask the simulator for the latest state *and* regular reports (defined above). The latest state is only really used to provide a smooth simulation playback. Disabling this may improve simulation performance (because the simulator will only have to post updates at the regular reporting interval).";
    };

    // stats collected whenever the simulation updates/reports
    struct SimStats final {
        // integrator stats
        float AccuracyInUse = 0.0f;
        static constexpr char const* g_AccuracyInUseDesc = "Get the accuracy which is being used for error control.  Usually this is the same value that was specified to setAccuracy()";

        float PredictedNextStepSize = 0.0f;
        static constexpr char const* g_PredictedNextStepSizeDesc = "Get the step size that will be attempted first on the next call to stepTo() or stepBy().";

        int NumStepsAttempted = 0;
        static constexpr char const* g_NumStepsAttemptedDesc = "Get the total number of steps that have been attempted (successfully or unsuccessfully)";

        int NumStepsTaken = 0;
        static constexpr char const* g_NumStepsTakenDesc = "Get the total number of steps that have been successfully taken";

        int NumRealizations = 0;
        static constexpr char const* g_NumRealizationsDesc = "Get the total number of state realizations that have been performed";

        int NumQProjections = 0;
        static constexpr char const* g_NumQProjectionsDesc = "Get the total number of times a state positions Q have been projected";

        int NumUProjections = 0;
        static constexpr char const* g_NumUProjectionsDesc = "Get the total number of times a state velocities U have been projected";

        int NumErrorTestFailures = 0;
        static constexpr char const* g_NumErrorTestFailuresDesc = "Get the number of attempted steps that have failed due to the error being unacceptably high";

        int NumConvergenceTestFailures = 0;
        static constexpr char const* g_NumConvergenceTestFailuresDesc = "Get the number of attempted steps that failed due to non-convergence of internal step iterations. This is most common with iterative methods but can occur if for some reason a step can't be completed.";

        int NumRealizationFailures = 0;
        static constexpr char const* g_NumRealizationFailuresDesc = "Get the number of attempted steps that have failed due to an error when realizing the state";

        int NumQProjectionFailures = 0;
        static constexpr char const* g_NumQProjectionFailuresDesc = "Get the number of attempted steps that have failed due to an error when projecting the state positions (Q)";

        int NumUProjectionFailures = 0;
        static constexpr char const* g_NumUProjectionFailuresDesc = "Get the number of attempted steps that have failed due to an error when projecting the state velocities (U)";

        int NumProjectionFailures = 0;
        static constexpr char const* g_NumProjectionFailuresDesc = "Get the number of attempted steps that have failed due to an error when projecting the state (either a Q- or U-projection)";

        int NumConvergentIterations = 0;
        static constexpr char const* g_NumConvergentIterationsDesc = "For iterative methods, get the number of internal step iterations in steps that led to convergence (not necessarily successful steps).";

        int NumDivergentIterations = 0;
        static constexpr char const* g_NumDivergentIterationsDesc = "For iterative methods, get the number of internal step iterations in steps that did not lead to convergence.";

        int NumIterations = 0;
        static constexpr char const* g_NumIterationsDesc = "For iterative methods, this is the total number of internal step iterations taken regardless of whether those iterations led to convergence or to successful steps. This is the sum of the number of convergent and divergent iterations which are available separately.";


        // system stats
        int NumPrescribeQcalls = 0;
    };

    // report produced whenever
    //
    // - the "latest state" is empty (for .try_pop_latest_report)
    // - the next reporting interval is hit (for .pop_regular_reports)
    struct Report final {
        SimTK::State state;
        SimStats stats;
    };

    // fd simulation that immediately starts running on a background thread
    class FdSimulation final {
        struct Impl;
        std::unique_ptr<Impl> m_Impl;

    public:
        // starts the simulation on construction
        FdSimulation(BasicModelStatePair, FdParams const& params);
        FdSimulation(FdSimulation const&) = delete;
        FdSimulation(FdSimulation&&) noexcept;

        // automatically cancels + joins the simulation thread
        //
        // rougly equivalent to calling .stop() on the simulator
        ~FdSimulation() noexcept;

        FdSimulation& operator=(FdSimulation const&) = delete;
        FdSimulation& operator=(FdSimulation&&) noexcept;

        // tries to pop the latest report from the simulator
        //
        // returns std::nullopt if the simulator thread hasn't populated a report
        // yet (i.e. if an integration/reporting step hasn't happened since the
        // last call)
        [[nodiscard]] std::unique_ptr<Report> tryPopLatestReport();
        [[nodiscard]] int numLatestReportsPopped() const;

        // these values are accurate to within one report, or integration step
        // (because the backend can only update them that often)

        [[nodiscard]] bool isRunning() const;
        [[nodiscard]] std::chrono::duration<double> wallDuration() const;
        [[nodiscard]] std::chrono::duration<double> simCurrentTime() const;
        [[nodiscard]] std::chrono::duration<double> simFinalTime() const;
        [[nodiscard]] char const* statusDescription() const;

        // progress of simulation, which falls in the range [0.0, 1.0]
        [[nodiscard]] float progress() const;

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
        //
        // - e.g. simulating 1 second with a reporting interval of 0.1 seconds results in
        //        11 reports
        int popRegularReports(std::vector<std::unique_ptr<Report>>& append_out);

        // requests that the simulator stops
        //
        // this is only a request: the simulation may still be running some time after
        // this method returns
        void requestStop();

        // synchronously stop the simulation
        //
        // this method blocks until the simulation thread stops completely
        void stop();

        // get the params used to run this simulation
        [[nodiscard]] FdParams const& params() const;
    };
}
