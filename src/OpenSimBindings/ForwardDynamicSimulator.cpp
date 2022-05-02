#include "ForwardDynamicSimulator.hpp"

#include "src/OpenSimBindings/ForwardDynamicSimulatorParams.hpp"
#include "src/OpenSimBindings/IntegratorOutputExtractor.hpp"
#include "src/OpenSimBindings/IntegratorMethod.hpp"
#include "src/OpenSimBindings/MultiBodySystemOutputExtractor.hpp"
#include "src/OpenSimBindings/SimulationReport.hpp"
#include "src/OpenSimBindings/SimulationStatus.hpp"
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

namespace
{
    // exclusively owned input data
    class SimulatorThreadInput final {
    public:
        SimulatorThreadInput(osc::BasicModelStatePair modelState,
                             osc::ForwardDynamicSimulatorParams const& params,
                             std::function<void(osc::SimulationReport)> reportCallback) :
            m_ModelState{std::move(modelState)},
            m_Params{params},
            m_ReportCallback{std::move(reportCallback)}
        {
        }

        SimTK::MultibodySystem const& getMultiBodySystem() const { return m_ModelState.getModel().getMultibodySystem(); }
        SimTK::State const& getState() const { return m_ModelState.getState(); }
        osc::ForwardDynamicSimulatorParams const& getParams() const { return m_Params; }
        void emitReport(osc::SimulationReport report) { m_ReportCallback(std::move(report)); }

    private:
        osc::BasicModelStatePair m_ModelState;
        osc::ForwardDynamicSimulatorParams m_Params;
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

static std::vector<osc::OutputExtractor> CreateSimulatorOutputExtractors()
{
    std::vector<osc::OutputExtractor> rv;
    rv.reserve(osc::GetNumIntegratorOutputExtractors() + osc::GetNumMultiBodySystemOutputExtractors());

    for (int i = 0, len = osc::GetNumIntegratorOutputExtractors(); i < len; ++i)
    {
        rv.push_back(osc::GetIntegratorOutputExtractorDynamic(i));
    }

    for (int i = 0, len = osc::GetNumMultiBodySystemOutputExtractors(); i < len; ++i)
    {
        rv.push_back(osc::GetMultiBodySystemOutputExtractorDynamic(i));
    }

    return rv;
}

static std::vector<osc::OutputExtractor> const& GetSimulatorOutputExtractors()
{
    static std::vector<osc::OutputExtractor> const g_Outputs = CreateSimulatorOutputExtractors();
    return g_Outputs;
}

static std::unique_ptr<SimTK::Integrator> CreateInitializedIntegrator(SimulatorThreadInput const& input)
{
    osc::ForwardDynamicSimulatorParams const& params = input.getParams();

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
    osc::ForwardDynamicSimulatorParams const& params = input.getParams();

    // create + init an integrator
    std::unique_ptr<SimTK::Integrator> integ = CreateInitializedIntegrator(input);

    // create + init a timestepper for the integrator
    SimTK::TimeStepper ts{input.getMultiBodySystem(), *integ};
    ts.initialize(integ->getState());

    // inform observers that everything has been initialized and the sim is now running
    shared.setStatus(osc::SimulationStatus::Running);

    // immediately report t = start
    input.emitReport(osc::SimulationReport{input.getMultiBodySystem(), *integ});

    // integrate (t0..tfinal]
    osc::SimulationClock::time_point tStart = GetSimulationTime(*integ);
    osc::SimulationClock::time_point tLastReport = tStart;
    osc::SimulationClock::duration stepDur = params.ReportingInterval;
    int step = 1;
    while (!integ->isSimulationOver())
    {
        // check for cancellation requests
        if (stopToken.stop_requested())
        {
            return osc::SimulationStatus::Cancelled;
        }

        // calculate next reporting time
        osc::SimulationClock::time_point tNext = tStart + step*stepDur;

        // perform an integration step
        SimTK::Integrator::SuccessfulStepStatus timestepRv = ts.stepTo(tNext.time_since_epoch().count());

        // handle integrator response
        if (integ->isSimulationOver() &&
            integ->getTerminationReason() != SimTK::Integrator::ReachedFinalTime)
        {
            // simulation ended because of an error: report the error and exit
            std::string reason = integ->getTerminationReasonString(integ->getTerminationReason());
            return osc::SimulationStatus::Error;
        }
        else if (timestepRv == SimTK::Integrator::ReachedReportTime)
        {
            // report the step and continue
            input.emitReport(osc::SimulationReport{input.getMultiBodySystem(), *integ});
            tLastReport = GetSimulationTime(*integ);
            ++step;
            continue;
        }
        else if (timestepRv == SimTK::Integrator::EndOfSimulation)
        {
            // if the simulation endpoint is sufficiently ahead of the last report time
            // (1 % of step size), then *also* report the simulation end time. Otherwise,
            // assume that there's an adjacent-enough report
            osc::SimulationClock::time_point t = GetSimulationTime(*integ);
            if ((tLastReport + 0.01*stepDur) < t)
            {
                input.emitReport(osc::SimulationReport{input.getMultiBodySystem(), *integ});
                tLastReport = t;
            }
            break;
        }
        else
        {
            // loop back and do the next timestep
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

class osc::ForwardDynamicSimulator::Impl final {
public:
    Impl(BasicModelStatePair modelState,
        ForwardDynamicSimulatorParams const& params,
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

    ForwardDynamicSimulatorParams const& params() const
    {
        return m_SimulationParams;
    }

private:
    ForwardDynamicSimulatorParams m_SimulationParams;
    std::shared_ptr<SharedState> m_Shared;
    jthread m_SimulatorThread;
};



// public API

int osc::GetNumFdSimulatorOutputExtractors()
{
    return static_cast<int>(GetSimulatorOutputExtractors().size());
}

osc::OutputExtractor osc::GetFdSimulatorOutputExtractor(int idx)
{
    return GetSimulatorOutputExtractors().at(static_cast<size_t>(idx));
}

osc::ForwardDynamicSimulator::ForwardDynamicSimulator(BasicModelStatePair msp,
                                ForwardDynamicSimulatorParams const& params,
                                std::function<void(SimulationReport)> reportCallback) :
    m_Impl{std::make_unique<Impl>(std::move(msp), params, std::move(reportCallback))}
{
}

osc::ForwardDynamicSimulator::ForwardDynamicSimulator(ForwardDynamicSimulator&&) noexcept = default;
osc::ForwardDynamicSimulator& osc::ForwardDynamicSimulator::operator=(ForwardDynamicSimulator&&) noexcept = default;
osc::ForwardDynamicSimulator::~ForwardDynamicSimulator() noexcept = default;

osc::SimulationStatus osc::ForwardDynamicSimulator::getStatus() const
{
    return m_Impl->getStatus();
}

void osc::ForwardDynamicSimulator::requestStop()
{
    return m_Impl->requestStop();
}

void osc::ForwardDynamicSimulator::stop()
{
    return m_Impl->stop();
}

osc::ForwardDynamicSimulatorParams const& osc::ForwardDynamicSimulator::params() const
{
    return m_Impl->params();
}
