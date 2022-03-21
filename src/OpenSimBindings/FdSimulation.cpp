#include "FdSimulation.hpp"

#include "src/OpenSimBindings/SimulationReport.hpp"
#include "src/OpenSimBindings/IntegratorMethod.hpp"
#include "src/OpenSimBindings/SimulationStatus.hpp"
#include "src/OpenSimBindings/IntegratorOutput.hpp"
#include "src/OpenSimBindings/MultiBodySystemOutput.hpp"
#include "src/Platform/Log.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/Cpp20Shims.hpp"

#include <OpenSim/Common/Exception.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <SimTKsimbody.h>
#include <simmath/Integrator.h>

#include <atomic>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

static constexpr char const* g_FinalTimeTitle = "Final Time (sec)";
static constexpr char const* g_FinalTimeDesc = "The final time, in seconds, that the forward dynamic simulation should integrate up to";
static constexpr char const* g_IntegratorMethodUsedTitle = "Integrator Method";
static constexpr char const* g_IntegratorMethodUsedDesc = "The integrator that the forward dynamic simulator should use. OpenSim's default integrator is a good choice if you aren't familiar with the other integrators. Changing the integrator can have a large impact on the performance and accuracy of the simulation.";
static constexpr char const* g_ReportingIntervalTitle = "Reporting Interval (sec)";
static constexpr char const* g_ReportingIntervalDesc = "How often the simulator should emit a simulation report. This affects how many datapoints are collected for the animation, output values, etc.";
static constexpr char const* g_IntegratorStepLimitTitle = "Integrator Step Limit";
static constexpr char const* g_IntegratorStepLimitDesc = "The maximum number of *internal* steps that can be taken within a single call to the integrator's stepTo/stepBy function. This is mostly an internal engine concern, but can occasionally affect how often reports are emitted";
static constexpr char const* g_IntegratorMinimumStepSizeTitle = "Minimum Step Size (sec)";
static constexpr char const* g_IntegratorMinimumStepSizeDesc = "The minimum step size, in seconds, that the integrator must take during the simulation. Note: this is mostly only relevant for error-corrected integrators that change their step size dynamically as the simulation runs.";
static constexpr char const* g_IntegratorMaximumStepSizeTitle = "Maximum step size (sec)";
static constexpr char const* g_IntegratorMaximumStepSizeDesc = "The maximum step size, in seconds, that the integrator must take during the simulation. Note: this is mostly only relevant for error-correct integrators that change their step size dynamically as the simulation runs";
static constexpr char const* g_IntegratorAccuracyTitle = "Accuracy";
static constexpr char const* g_IntegratorAccuracyDesc = "Target accuracy for the integrator. Mostly only relevant for error-controlled integrators that change their step size by comparing this accuracy value to measured integration error";

namespace
{
    // exclusively owned input data
    class SimulatorThreadInput final {
    public:
        SimulatorThreadInput(osc::BasicModelStatePair modelState,
                             osc::FdParams const& params,
                             std::function<void(osc::SimulationReport)> reportCallback) :
            m_ModelState{std::move(modelState)},
            m_Params{params},
            m_ReportCallback{std::move(reportCallback)}
        {
        }

        SimTK::MultibodySystem const& getMultiBodySystem() const { return m_ModelState.getModel().getMultibodySystem(); }
        SimTK::State const& getState() const { return m_ModelState.getState(); }
        SimTK::State& updState() { return m_ModelState.updState(); }
        osc::FdParams const& getParams() const { return m_Params; }
        void emitReport(osc::SimulationReport report) { m_ReportCallback(std::move(report)); }

    private:
        osc::BasicModelStatePair m_ModelState;
        osc::FdParams m_Params;
        std::function<void(osc::SimulationReport)> m_ReportCallback;
    };

    // data that's shared with the UI thread
    class SharedState final {
    public:
        osc::SimulationStatus getStatus() const
        {
            return static_cast<osc::SimulationStatus>(m_Status.load());
        }

        void setStatus(osc::SimulationStatus s)
        {
            m_Status = static_cast<int>(s);
        }
    private:
        std::atomic<int> m_Status = static_cast<int>(osc::SimulationStatus::Initializing);
    };
}

static std::vector<osc::Output> CreateSimulatorOutputs()
{
    std::vector<osc::Output> rv;
    rv.reserve(osc::GetNumIntegratorOutputs() + osc::GetNumMultiBodySystemOutputs());

    for (int i = 0, len = osc::GetNumIntegratorOutputs(); i < len; ++i)
    {
        rv.push_back(osc::GetIntegratorOutputDynamic(i));
    }

    for (int i = 0, len = osc::GetNumMultiBodySystemOutputs(); i < len; ++i)
    {
        rv.push_back(osc::GetMultiBodySystemOutputDynamic(i));
    }

    return rv;
}

static std::vector<osc::Output> const& GetSimulatorOutputs()
{
    static std::vector<osc::Output> const g_Outputs = CreateSimulatorOutputs();
    return g_Outputs;
}

static std::unique_ptr<SimTK::Integrator> CreateInitializedIntegrator(SimulatorThreadInput const& input)
{
    osc::FdParams const& params = input.getParams();

    // create + init an integrator
    auto integ = CreateIntegrator(input.getMultiBodySystem(), params.IntegratorMethodUsed);
    integ->setInternalStepLimit(params.IntegratorStepLimit);
    integ->setMinimumStepSize(params.IntegratorMinimumStepSize.count());
    integ->setMaximumStepSize(params.IntegratorMaximumStepSize.count());
    integ->setAccuracy(params.IntegratorAccuracy);
    integ->setFinalTime(params.FinalTime.time_since_epoch().count());
    integ->initialize(input.getState());
    return integ;
}

static osc::SimulationClock::time_point GetSimulationTime(SimTK::Integrator const& integ)
{
    return osc::SimulationClock::time_point(osc::SimulationClock::duration(integ.getTime()));
}

// this is the main function that the simulator thread works through (unguarded against exceptions)
static osc::SimulationStatus FdSimulationMainUnguarded(osc::stop_token stopToken,
                                                       SimulatorThreadInput& input,
                                                       SharedState& shared)
{
    osc::FdParams const& params = input.getParams();

    // create + init an integrator
    std::unique_ptr<SimTK::Integrator> integ = CreateInitializedIntegrator(input);

    // create + init a timestepper for the integrator
    SimTK::TimeStepper ts{input.getMultiBodySystem(), *integ};
    ts.initialize(integ->getState());

    // figure out timesteps the sim should use
    osc::SimulationClock::time_point t0 = GetSimulationTime(*integ);
    osc::SimulationClock::time_point tFinal = params.FinalTime;
    osc::SimulationClock::time_point tNextReport = t0 + params.ReportingInterval;

    // tell the UI thread that everything has been initialized and it's now running
    shared.setStatus(osc::SimulationStatus::Running);

    // immediately report t0
    input.emitReport(osc::SimulationReport{input.getMultiBodySystem(), *integ});

    // integrate (t0..tfinal]
    for (osc::SimulationClock::time_point t = t0; t < tFinal; t = GetSimulationTime(*integ))
    {
        // check for cancellation requests
        if (stopToken.stop_requested())
        {
            return osc::SimulationStatus::Cancelled;
        }

        // simulate an integration step (expensive)
        auto nextTimepoint = std::min(tNextReport, tFinal);
        auto timestepRv = ts.stepTo(nextTimepoint.time_since_epoch().count());

        // handle integration errors
        if (integ->isSimulationOver() &&
            integ->getTerminationReason() != SimTK::Integrator::ReachedFinalTime)
        {
            std::string reason = integ->getTerminationReasonString(integ->getTerminationReason());
            osc::log::error("simulation error: integration failed: %s", reason.c_str());
            return osc::SimulationStatus::Error;
        }

        // skip uninteresting integration steps
        if (timestepRv != SimTK::Integrator::TimeHasAdvanced &&
            timestepRv != SimTK::Integrator::ReachedScheduledEvent &&
            timestepRv != SimTK::Integrator::ReachedReportTime &&
            timestepRv != SimTK::Integrator::ReachedStepLimit)
        {
            continue;
        }

        // emit report (if necessary)
        osc::SimulationClock::time_point tInteg = GetSimulationTime(*integ);
        if (osc::IsEffectivelyEqual(nextTimepoint.time_since_epoch().count(), tInteg.time_since_epoch().count()))
        {
            input.emitReport(osc::SimulationReport{input.getMultiBodySystem(), *integ});
            tNextReport = tInteg + params.ReportingInterval;
        }
    }

    return osc::SimulationStatus::Completed;
}

// MAIN function for the simulator thread
//
// guarded against exceptions (which are handled as simulation failures)
static int FdSimulationMain(osc::stop_token stopToken,
                            std::unique_ptr<SimulatorThreadInput> input,
                            std::shared_ptr<SharedState> shared)
{
    osc::SimulationStatus status = osc::SimulationStatus::Error;

    try
    {
        status = FdSimulationMainUnguarded(std::move(stopToken), *input, *shared);
    }
    catch (OpenSim::Exception const& ex)
    {
        osc::log::error("OpenSim::Exception occurred when running a simulation: %s", ex.what());
    }
    catch (std::exception const& ex)
    {
        osc::log::error("std::exception occurred when running a simulation: %s", ex.what());
    }
    catch (...)
    {
        osc::log::error("an exception with unknown type occurred when running a simulation (no error message available)");
    }

    shared->setStatus(status);

    return 0;
}

osc::ParamBlock osc::ToParamBlock(FdParams const& p)
{
    ParamBlock rv;
    rv.pushParam(g_FinalTimeTitle, g_FinalTimeDesc, (p.FinalTime - SimulationClock::start()).count());
    rv.pushParam(g_IntegratorMethodUsedTitle, g_IntegratorMethodUsedDesc, p.IntegratorMethodUsed);
    rv.pushParam(g_ReportingIntervalTitle, g_ReportingIntervalDesc, p.ReportingInterval.count());
    rv.pushParam(g_IntegratorStepLimitTitle, g_IntegratorStepLimitDesc, p.IntegratorStepLimit);
    rv.pushParam(g_IntegratorMinimumStepSizeTitle, g_IntegratorMinimumStepSizeDesc, p.IntegratorMinimumStepSize.count());
    rv.pushParam(g_IntegratorMaximumStepSizeTitle, g_IntegratorMaximumStepSizeDesc, p.IntegratorMaximumStepSize.count());
    rv.pushParam(g_IntegratorAccuracyTitle, g_IntegratorAccuracyDesc, p.IntegratorAccuracy);
    return rv;
}

osc::FdParams osc::FromParamBlock(ParamBlock const& b)
{
    FdParams rv;
    if (auto finalTime = b.findValue(g_FinalTimeTitle); finalTime && std::holds_alternative<double>(*finalTime))
    {
        rv.FinalTime = SimulationClock::start() + SimulationClock::duration{std::get<double>(*finalTime)};
    }
    if (auto integMethod = b.findValue(g_IntegratorMethodUsedTitle); integMethod && std::holds_alternative<IntegratorMethod>(*integMethod))
    {
        rv.IntegratorMethodUsed = std::get<IntegratorMethod>(*integMethod);
    }
    if (auto repInterv = b.findValue(g_ReportingIntervalTitle); repInterv && std::holds_alternative<double>(*repInterv))
    {
        rv.ReportingInterval = SimulationClock::duration{std::get<double>(*repInterv)};
    }
    if (auto stepLim = b.findValue(g_IntegratorStepLimitTitle); stepLim && std::holds_alternative<int>(*stepLim))
    {
        rv.IntegratorStepLimit = std::get<int>(*stepLim);
    }
    if (auto minStep = b.findValue(g_IntegratorMinimumStepSizeTitle); minStep && std::holds_alternative<double>(*minStep))
    {
        rv.IntegratorMinimumStepSize = SimulationClock::duration{std::get<double>(*minStep)};
    }
    if (auto maxStep = b.findValue(g_IntegratorMaximumStepSizeTitle); maxStep && std::holds_alternative<double>(*maxStep))
    {
        rv.IntegratorMaximumStepSize = SimulationClock::duration{std::get<double>(*maxStep)};
    }
    if (auto acc = b.findValue(g_IntegratorAccuracyTitle); acc && std::holds_alternative<double>(*acc))
    {
        rv.IntegratorAccuracy = std::get<double>(*acc);
    }
    return rv;
}

class osc::FdSimulation::Impl final {
public:
    Impl(BasicModelStatePair modelState,
         FdParams const& params,
         std::function<void(SimulationReport)> reportCallback) :

        m_SimulationParams{params},
        m_Shared{std::make_shared<SharedState>()},
        m_SimulatorThread
        {
            FdSimulationMain,
            std::make_unique<SimulatorThreadInput>(std::move(modelState),
                                                   params,
                                                   std::move(reportCallback)),
            m_Shared
        }
    {
    }

    SimulationStatus getStatus() const
    {
        return m_Shared->getStatus();
    }

    void requestStop()
    {
        m_SimulatorThread.request_stop();
    }

    void stop()
    {
        m_SimulatorThread.request_stop();
        m_SimulatorThread.join();
    }

    FdParams const& params() const
    {
        return m_SimulationParams;
    }

private:
    FdParams m_SimulationParams;
    std::shared_ptr<SharedState> m_Shared;
    jthread m_SimulatorThread;
};



// public API

int osc::GetNumFdSimulatorOutputs()
{
    return static_cast<int>(GetSimulatorOutputs().size());
}

osc::Output osc::GetFdSimulatorOutputDynamic(int idx)
{
    return GetSimulatorOutputs().at(static_cast<size_t>(idx));
}

osc::FdSimulation::FdSimulation(BasicModelStatePair msp,
                                FdParams const& params,
                                std::function<void(SimulationReport)> reportCallback) :
    m_Impl{std::make_unique<Impl>(std::move(msp), params, std::move(reportCallback))}
{
}

osc::FdSimulation::FdSimulation(FdSimulation&&) noexcept = default;
osc::FdSimulation& osc::FdSimulation::operator=(FdSimulation&&) noexcept = default;
osc::FdSimulation::~FdSimulation() noexcept = default;

osc::SimulationStatus osc::FdSimulation::getStatus() const
{
    return m_Impl->getStatus();
}

void osc::FdSimulation::requestStop()
{
    return m_Impl->requestStop();
}

void osc::FdSimulation::stop()
{
    return m_Impl->stop();
}

osc::FdParams const& osc::FdSimulation::params() const
{
    return m_Impl->params();
}
