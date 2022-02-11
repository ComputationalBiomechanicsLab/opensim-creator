#include "Simulation.hpp"

#include "src/Log.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/ConcurrencyHelpers.hpp"
#include "src/Utils/Cpp20Shims.hpp"
#include "src/Assertions.hpp"
#include "src/App.hpp"

#include <OpenSim/Simulation/Manager/Manager.h>
#include <OpenSim/Simulation/Model/Analysis.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <SimTKsimbody.h>
#include <simmath/Integrator.h>

#include <functional>
#include <ratio>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

using namespace osc;

// data

IntegratorMethod const osc::g_IntegratorMethods[IntegratorMethod_NumIntegratorMethods] = {
    IntegratorMethod_OpenSimManagerDefault,
    IntegratorMethod_ExplicitEuler,
    IntegratorMethod_RungeKutta2,
    IntegratorMethod_RungeKutta3,
    IntegratorMethod_RungeKuttaFeldberg,
    IntegratorMethod_RungeKuttaMerson,
    IntegratorMethod_SemiExplicitEuler2,
    IntegratorMethod_Verlet,
};

char const* const osc::g_IntegratorMethodNames[IntegratorMethod_NumIntegratorMethods] = {
    "OpenSim::Manager Default",
    "Explicit Euler",
    "Runge Kutta 2",
    "Runge Kutta 3",
    "Runge Kutta Feldberg",
    "Runge Kutta Merson",
    "Semi Explicit Euler 2",
    "Verlet",
};

// simulation status - returned by simulator thread
enum class FdsimStatus {
    Running = 1,
    Completed,
    Cancelled,
    Error,
};

// convert an IntegratorMethod (enum) into a SimTK::Integrator
static SimTK::Integrator* fdsimMakeIntegratorUnguarded(SimTK::System const& system, IntegratorMethod method) {
    switch (method) {
    case IntegratorMethod_OpenSimManagerDefault:
        return new SimTK::RungeKuttaMersonIntegrator{system};
    case IntegratorMethod_ExplicitEuler:
        return new SimTK::ExplicitEulerIntegrator{system};
    case IntegratorMethod_RungeKutta2:
        return new SimTK::RungeKutta2Integrator{system};
    case IntegratorMethod_RungeKutta3:
        return new SimTK::RungeKutta3Integrator{system};
    case IntegratorMethod_RungeKuttaFeldberg:
        return new SimTK::RungeKuttaFeldbergIntegrator{system};
    case IntegratorMethod_RungeKuttaMerson:
        return new SimTK::RungeKuttaMersonIntegrator{system};
    case IntegratorMethod_SemiExplicitEuler2:
        return new SimTK::SemiExplicitEuler2Integrator{system};
    case IntegratorMethod_Verlet:
        return new SimTK::VerletIntegrator{system};
    default:
        return new SimTK::RungeKuttaMersonIntegrator{system};
    }
}

// convert an IntegratorMethod (enum) into a std::unique_ptr<SimTK::Integrator>
static std::unique_ptr<SimTK::Integrator> fdsimMakeIntegrator(SimTK::System const& system, IntegratorMethod method) {
    return std::unique_ptr<SimTK::Integrator>{fdsimMakeIntegratorUnguarded(system, method)};
}

namespace {
    // state that is shared between UI and simulation thread
    struct SharedState final {
        FdsimStatus status = FdsimStatus::Running;

        std::chrono::steady_clock::time_point wallStart = std::chrono::steady_clock::now();
        std::chrono::steady_clock::time_point wallEnd = std::chrono::steady_clock::now();  // only updated at the end
        std::chrono::duration<double> latestSimTime{0};

        std::unique_ptr<Report> latestReport = nullptr;
        std::vector<std::unique_ptr<Report>> regularReports;
    };
}

// generate a report from the integrator's current state
static std::unique_ptr<Report> makeSimulationReport(OpenSim::Model const& model, SimTK::Integrator const& integrator) {
    std::unique_ptr<Report> out = std::make_unique<Report>();
    out->state = integrator.getState();

    SimStats& stats = out->stats;

    // integrator stats
    stats.AccuracyInUse = static_cast<float>(integrator.getAccuracyInUse());
    stats.PredictedNextStepSize = static_cast<float>(integrator.getPredictedNextStepSize());
    stats.NumStepsAttempted = integrator.getNumStepsAttempted();
    stats.NumStepsTaken = integrator.getNumStepsTaken();
    stats.NumRealizations = integrator.getNumRealizations();
    stats.NumQProjections = integrator.getNumQProjections();
    stats.NumUProjections = integrator.getNumUProjections();
    stats.NumErrorTestFailures = integrator.getNumErrorTestFailures();
    stats.NumConvergenceTestFailures = integrator.getNumConvergenceTestFailures();
    stats.NumRealizationFailures = integrator.getNumRealizationFailures();
    stats.NumQProjectionFailures = integrator.getNumQProjectionFailures();
    stats.NumUProjectionFailures = integrator.getNumUProjectionFailures();
    stats.NumProjectionFailures = integrator.getNumProjectionFailures();
    stats.NumConvergentIterations = integrator.getNumConvergentIterations();
    stats.NumDivergentIterations = integrator.getNumDivergentIterations();
    stats.NumIterations = integrator.getNumIterations();

    // system stats
    stats.NumPrescribeQcalls =  model.getSystem().getNumPrescribeQCalls();

    return out;
}

// MAIN function for the simulator thread
//
// unguarded from exceptions
static FdsimStatus FdSimulationMainUnguarded(
        stop_token stopToken,
        std::unique_ptr<Input> input,
        std::shared_ptr<Mutex_guarded<SharedState>> shared) {

    OpenSim::Model& model = *input->model;
    SimTK::State& state = *input->state;
    FdParams const& params = input->params;

    // create + init an integrator
    std::unique_ptr<SimTK::Integrator> integ =
        fdsimMakeIntegrator(input->model->getMultibodySystem(), input->params.IntegratorMethodUsed);
    integ->setInternalStepLimit(params.IntegratorStepLimit);
    integ->setMinimumStepSize(params.IntegratorMinimumStepSize.count());
    integ->setMaximumStepSize(params.IntegratorMaximumStepSize.count());
    integ->setAccuracy(params.IntegratorAccuracy);
    integ->setFinalTime(params.FinalTime.count());
    integ->setReturnEveryInternalStep(params.UpdateLatestStateOnEveryStep);
    integ->initialize(state);

    // create + init a timestepper for the integrator
    SimTK::TimeStepper ts{model.getMultibodySystem(), *integ};
    ts.initialize(integ->getState());
    ts.setReportAllSignificantStates(params.UpdateLatestStateOnEveryStep);

    // figure out timesteps the sim should use
    double t0 = integ->getTime();
    auto t0WallTime = std::chrono::steady_clock::now();
    double tFinalSimTime = params.FinalTime.count();
    double tNextRegularReport = t0;

    // immediately report t0
    {
        std::unique_ptr<Report> regularReport = makeSimulationReport(model, *integ);
        std::unique_ptr<Report> spotReport = makeSimulationReport(model, *integ);

        auto guard = shared->lock();
        guard->regularReports.push_back(std::move(regularReport));
        guard->latestReport = std::move(spotReport);
        tNextRegularReport = t0 + params.ReportingInterval.count();
        App::cur().requestRedraw();
    }

    // integrate (t0..tfinal]
    for (double t = t0; t < tFinalSimTime; t = integ->getTime()) {

        // check for thread cancellation requests
        if (stopToken.stop_requested()) {
            return FdsimStatus::Cancelled;
        }

        // handle CPU throttling
        if (params.ThrottleToWallTime) {
            double dt = t - t0;
            std::chrono::microseconds dtSimMicros =
                std::chrono::microseconds{static_cast<long>(1000000.0 * dt)};

            auto tWall = std::chrono::steady_clock::now();
            auto dtWall = tWall - t0WallTime;
            auto dtWallMicros =
                std::chrono::duration_cast<std::chrono::microseconds>(dtWall);

            if (dtSimMicros > dtWallMicros) {
                std::this_thread::sleep_for(dtSimMicros - dtWallMicros);
            }
        }

        // compute an integration step
        auto nextTimepoint = std::min(tNextRegularReport, tFinalSimTime);
        auto timestepRv = ts.stepTo(nextTimepoint);

        // handle integration errors
        if (integ->isSimulationOver() &&
            integ->getTerminationReason() != SimTK::Integrator::ReachedFinalTime) {

            std::string reason = integ->getTerminationReasonString(integ->getTerminationReason());
            log::error("simulation error: integration failed: %s", reason.c_str());
            return FdsimStatus::Error;
        }

        // skip uninteresting integration steps
        if (timestepRv != SimTK::Integrator::TimeHasAdvanced &&
            timestepRv != SimTK::Integrator::ReachedScheduledEvent &&
            timestepRv != SimTK::Integrator::ReachedReportTime &&
            timestepRv != SimTK::Integrator::ReachedStepLimit) {

            continue;
        }

        // report interesting integration steps
        {
            std::unique_ptr<Report> regulaReport = nullptr;
            std::unique_ptr<Report> spotReport = nullptr;

            // create regular report (if necessary)
            if (AreEffectivelyEqual(nextTimepoint, integ->getTime())) {
                regulaReport = makeSimulationReport(model, *integ);
                tNextRegularReport = integ->getTime() + params.ReportingInterval.count();
            }

            // create spot report (if necessary)
            if (shared->lock()->latestReport == nullptr) {
                spotReport = makeSimulationReport(model, *integ);
            }

            // throw the reports over the fence to the calling thread
            auto guard = shared->lock();
            guard->latestSimTime = std::chrono::duration<double>{integ->getTime()};
            if (regulaReport) {
                guard->regularReports.push_back(std::move(regulaReport));
                App::cur().requestRedraw();
            }
            if (spotReport) {
                guard->latestReport = std::move(spotReport);
                App::cur().requestRedraw();
            }
        }
    }

    return FdsimStatus::Completed;
}

// MAIN function for the simulator thread
//
// guarded against exceptions (which are handled as simulation failures)
static int FdSimulationMain(
    stop_token stopToken,
    std::unique_ptr<Input> input,
    std::shared_ptr<Mutex_guarded<SharedState>> shared) {

    FdsimStatus status = FdsimStatus::Error;

    try {
        status = FdSimulationMainUnguarded(std::move(stopToken), std::move(input), shared);
    } catch (OpenSim::Exception const& ex) {
        log::error("OpenSim::Exception occurred when running a simulation: %s", ex.what());
    } catch (std::exception const& ex) {
        log::error("std::exception occurred when running a simulation: %s", ex.what());
    } catch (...) {
        log::error("an exception with unknown type occurred when running a simulation (no error message available)");
    }

    auto guard = shared->lock();
    guard->wallEnd = std::chrono::steady_clock::now();
    guard->status = status;

    return 0;
}

// simulation: internal state
struct osc::FdSimulation::Impl final {

    // caller-side copy of sim params
    FdParams params;

    // mutex-guarded state shared between the caller and sim thread
    std::shared_ptr<Mutex_guarded<SharedState>> shared;

    // the sim thread
    jthread simulatorThread;

    // number of states popped from the sim thread
    int numStatesPopped;

    Impl(std::unique_ptr<Input> input) :
        params{input->params},

        shared{new Mutex_guarded<SharedState>{}},

        // starts the simulation
        simulatorThread{FdSimulationMain, std::move(input), shared},

        numStatesPopped{0} {
    }
};

// public API

osc::Input::Input(std::unique_ptr<OpenSim::Model> _model, std::unique_ptr<SimTK::State> _state) :
    model{std::move(_model)}, state{std::move(_state)} {
}

osc::FdSimulation::FdSimulation(std::unique_ptr<Input> input) :
    m_Impl{new Impl{std::move(input)}} {
}

osc::FdSimulation::FdSimulation(FdSimulation&&) noexcept = default;

osc::FdSimulation::~FdSimulation() noexcept = default;

osc::FdSimulation& osc::FdSimulation::FdSimulation::operator=(FdSimulation&&) noexcept = default;

std::unique_ptr<Report> osc::FdSimulation::tryPopLatestReport() {
    std::unique_ptr<Report> rv = nullptr;

    {
        auto guard = m_Impl->shared->lock();
        rv = std::move(guard->latestReport);
    }

    if (rv) {
        ++m_Impl->numStatesPopped;
    }

    return rv;
}

int osc::FdSimulation::numLatestReportsPopped() const {
    return m_Impl->numStatesPopped;
}

bool osc::FdSimulation::isRunning() const {
    auto guard = m_Impl->shared->lock();
    return guard->status == FdsimStatus::Running;
}

std::chrono::duration<double> osc::FdSimulation::wallDuration() const {
    auto guard = m_Impl->shared->lock();
    auto end = guard->status == FdsimStatus::Running ? std::chrono::steady_clock::now() : guard->wallEnd;
    return end - guard->wallEnd;
}

std::chrono::duration<double> osc::FdSimulation::simCurrentTime() const {
    auto guard = m_Impl->shared->lock();
    return guard->latestSimTime;
}

std::chrono::duration<double> osc::FdSimulation::simFinalTime() const {
    return m_Impl->params.FinalTime;
}

char const* osc::FdSimulation::statusDescription() const {
    switch (m_Impl->shared->lock()->status) {
    case FdsimStatus::Running:
        return "running";
    case FdsimStatus::Completed:
        return "completed";
    case FdsimStatus::Cancelled:
        return "cancelled";
    case FdsimStatus::Error:
        return "error";
    default:
        return "UNKNOWN STATUS: DEV ERROR";
    }
}

float osc::FdSimulation::progress() const {
    double cur = simCurrentTime().count();
    double end = m_Impl->params.FinalTime.count();
    return static_cast<float>(cur) / static_cast<float>(end);
}

int osc::FdSimulation::popRegularReports(std::vector<std::unique_ptr<Report>>& append_out) {
    auto guard = m_Impl->shared->lock();

    for (auto& ptr : guard->regularReports) {
        append_out.push_back(std::move(ptr));
    }

    OSC_ASSERT(guard->regularReports.size() < std::numeric_limits<int>::max());
    int popped = static_cast<int>(guard->regularReports.size());

    guard->regularReports.clear();

    return popped;
}

void osc::FdSimulation::requestStop() {
    m_Impl->simulatorThread.request_stop();
}

void osc::FdSimulation::stop() {
    m_Impl->simulatorThread.request_stop();
    m_Impl->simulatorThread.join();
}

FdParams const& osc::FdSimulation::params() const {
    return m_Impl->params;
}
