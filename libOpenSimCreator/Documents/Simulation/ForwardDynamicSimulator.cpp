#include "ForwardDynamicSimulator.h"

#include <libOpenSimCreator/Documents/Model/BasicModelStatePair.h>
#include <libOpenSimCreator/Documents/OutputExtractors/IOutputExtractor.h>
#include <libOpenSimCreator/Documents/OutputExtractors/IntegratorOutputExtractor.h>
#include <libOpenSimCreator/Documents/OutputExtractors/MultiBodySystemOutputExtractor.h>
#include <libOpenSimCreator/Documents/Simulation/ForwardDynamicSimulatorParams.h>
#include <libOpenSimCreator/Documents/Simulation/IntegratorMethod.h>
#include <libOpenSimCreator/Documents/Simulation/SimulationClock.h>
#include <libOpenSimCreator/Documents/Simulation/SimulationReport.h>
#include <libOpenSimCreator/Documents/Simulation/SimulationStatus.h>

#include <liboscar/Platform/Log.h>
#include <liboscar/Shims/Cpp20/stop_token.h>
#include <liboscar/Shims/Cpp20/thread.h>
#include <liboscar/Utils/HashHelpers.h>
#include <liboscar/Utils/UID.h>
#include <OpenSim/Common/Exception.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <simmath/Integrator.h>
#include <simmath/TimeStepper.h>
#include <SimTKsimbody.h>

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <exception>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace OpenSim { class Component; }

namespace cpp20 = osc::cpp20;
using namespace osc;

namespace
{
    UID GetWalltimeUID()
    {
        static const UID s_WalltimeUID;
        return s_WalltimeUID;
    }

    UID GetStepDurationUID()
    {
        static const UID s_StepDurationUID;
        return s_StepDurationUID;
    }

    // exclusively owned input data
    class SimulatorThreadInput final {
    public:
        SimulatorThreadInput(BasicModelStatePair modelState,
                             const ForwardDynamicSimulatorParams& params,
                             std::function<void(SimulationReport)> onReportFromBgThread) :
            m_ModelState{std::move(modelState)},
            m_Params{params},
            m_ReportCallback{std::move(onReportFromBgThread)}
        {
        }

        const SimTK::MultibodySystem& getMultiBodySystem() const { return m_ModelState.getModel().getMultibodySystem(); }
        const SimTK::State& getState() const { return m_ModelState.getState(); }
        const ForwardDynamicSimulatorParams& getParams() const { return m_Params; }
        void emitReport(SimulationReport report) { m_ReportCallback(std::move(report)); }

    private:
        BasicModelStatePair m_ModelState;
        ForwardDynamicSimulatorParams m_Params;
        std::function<void(SimulationReport)> m_ReportCallback;
    };

    // data that's shared with the UI thread
    class SharedState final {
    public:
        SimulationStatus getStatus() const
        {
            return static_cast<SimulationStatus>(m_Status.load());
        }

        void setStatus(SimulationStatus s)
        {
            m_Status = static_cast<int>(s);
        }
    private:
        std::atomic<int> m_Status = static_cast<int>(SimulationStatus::Initializing);
    };

    class AuxiliaryVariableOutputExtractor final : public IOutputExtractor {
    public:
        AuxiliaryVariableOutputExtractor(std::string name, std::string description, UID uid) :
            m_Name{std::move(name)},
            m_Description{std::move(description)},
            m_UID{uid}
        {}

    private:
        CStringView implGetName() const final
        {
            return m_Name;
        }

        CStringView implGetDescription() const final
        {
            return m_Description;
        }

        OutputExtractorDataType implGetOutputType() const final
        {
            return OutputExtractorDataType::Float;
        }

        OutputValueExtractor implGetOutputValueExtractor(const OpenSim::Component&) const final
        {
            return OutputValueExtractor{[id = m_UID](const SimulationReport& report)
            {
                return Variant{report.getAuxiliaryValue(id).value_or(-1337.0f)};
            }};
        }

        std::size_t implGetHash() const final
        {
            return hash_of(m_Name, m_Description, m_UID);
        }

        bool implEquals(const IOutputExtractor& other) const final
        {
            if (&other == this)
            {
                return true;
            }

            const auto* const otherT = dynamic_cast<const AuxiliaryVariableOutputExtractor*>(&other);
            if (!otherT)
            {
                return false;
            }

            return
                m_Name == otherT->m_Name &&
                m_Description == otherT->m_Description &&
                m_UID == otherT->m_UID;
        }

        std::string m_Name;
        std::string m_Description;
        UID m_UID;
    };

    std::vector<OutputExtractor> CreateSimulatorOutputExtractors()
    {
        std::vector<OutputExtractor> rv;
        rv.reserve(static_cast<size_t>(2) + GetNumIntegratorOutputExtractors() + GetNumMultiBodySystemOutputExtractors());

        {
            const OutputExtractor out{AuxiliaryVariableOutputExtractor{
                "Wall time",
                "Total cumulative time spent computing the simulation",
                GetWalltimeUID(),
            }};
            rv.push_back(out);

            const OutputExtractor out2{AuxiliaryVariableOutputExtractor{
                "Step Wall Time",
                "How long it took, in wall time, to compute the last integration step",
                GetStepDurationUID(),
            }};
            rv.push_back(out2);
        }

        for (int i = 0, len = GetNumIntegratorOutputExtractors(); i < len; ++i)
        {
            rv.push_back(GetIntegratorOutputExtractorDynamic(i));
        }

        for (int i = 0, len = GetNumMultiBodySystemOutputExtractors(); i < len; ++i)
        {
            rv.push_back(GetMultiBodySystemOutputExtractorDynamic(i));
        }

        return rv;
    }

    std::vector<OutputExtractor> const& GetSimulatorOutputExtractors()
    {
        static const std::vector<OutputExtractor> s_Outputs = CreateSimulatorOutputExtractors();
        return s_Outputs;
    }

    std::unique_ptr<SimTK::Integrator> CreateInitializedIntegrator(const SimulatorThreadInput& input)
    {
        const ForwardDynamicSimulatorParams& params = input.getParams();

        // create + init an integrator
        auto integ = params.integratorMethodUsed.instantiate(input.getMultiBodySystem());
        integ->setInternalStepLimit(params.integratorStepLimit);
        integ->setMinimumStepSize(params.integratorMinimumStepSize.count());
        integ->setMaximumStepSize(params.integratorMaximumStepSize.count());
        integ->setAccuracy(params.integratorAccuracy);
        integ->setFinalTime(params.finalTime.time_since_epoch().count());
        integ->setReturnEveryInternalStep(true);  // so that cancellations/interrupts work
        integ->initialize(input.getState());
        return integ;
    }

    SimulationClock::time_point GetSimulationTime(const SimTK::Integrator& integ)
    {
        return SimulationClock::time_point(SimulationClock::duration(integ.getTime()));
    }

    SimulationReport CreateSimulationReport(
        std::chrono::duration<float> wallTime,
        std::chrono::duration<float> stepDuration,
        const SimTK::MultibodySystem& sys,
        const SimTK::Integrator& integrator)
    {
        SimTK::State st = integrator.getState();
        std::unordered_map<UID, float> auxValues;

        // care: state needs to be realized on the simulator thread
        st.invalidateAllCacheAtOrAbove(SimTK::Stage::Instance);

        // populate forward dynamic simulator outputs
        {
            auxValues.emplace(GetWalltimeUID(), wallTime.count());
            auxValues.emplace(GetStepDurationUID(), stepDuration.count());
        }

        // populate integrator outputs
        {
            const int numOutputs = GetNumIntegratorOutputExtractors();
            auxValues.reserve(auxValues.size() + numOutputs);
            for (int i = 0; i < numOutputs; ++i)
            {
                const IntegratorOutputExtractor& o = GetIntegratorOutputExtractor(i);
                auxValues.emplace(o.getAuxiliaryDataID(), o.getExtractorFunction()(integrator));
            }
        }

        // populate mbs outputs
        {
            const int numOutputs = GetNumMultiBodySystemOutputExtractors();
            auxValues.reserve(auxValues.size() + numOutputs);
            for (int i = 0; i < numOutputs; ++i)
            {
                const MultiBodySystemOutputExtractor& o = GetMultiBodySystemOutputExtractor(i);
                auxValues.emplace(o.getAuxiliaryDataID(), o.getExtractorFunction()(sys));
            }
        }

        return SimulationReport{std::move(st), std::move(auxValues)};
    }

    // this is the main function that the simulator thread works through (unguarded against exceptions)
    SimulationStatus FdSimulationMainUnguarded(
        cpp20::stop_token stopToken,
        SimulatorThreadInput& input,
        SharedState& shared)
    {
        const std::chrono::high_resolution_clock::time_point tSimStart = std::chrono::high_resolution_clock::now();

        const ForwardDynamicSimulatorParams& params = input.getParams();

        // create + init an integrator
        std::unique_ptr<SimTK::Integrator> integ = CreateInitializedIntegrator(input);

        // create + init a timestepper for the integrator
        SimTK::TimeStepper ts{input.getMultiBodySystem(), *integ};
        ts.initialize(integ->getState());
        ts.setReportAllSignificantStates(true);  // so that cancellations/interrupts work

        // inform observers that everything has been initialized and the sim is now running
        shared.setStatus(SimulationStatus::Running);

        // immediately report t = start
        {
            const std::chrono::duration<float> wallDur = std::chrono::high_resolution_clock::now() - tSimStart;
            input.emitReport(CreateSimulationReport(wallDur, {}, input.getMultiBodySystem(), *integ));
        }

        // integrate (t0..tfinal]
        const SimulationClock::time_point tStart = GetSimulationTime(*integ);
        SimulationClock::time_point tLastReport = tStart;
        int step = 1;
        while (!integ->isSimulationOver())
        {
            // check for cancellation requests
            if (stopToken.stop_requested())
            {
                return SimulationStatus::Cancelled;
            }

            // calculate next reporting time
            const SimulationClock::time_point tNext = tStart + step*params.reportingInterval;

            // perform an integration step
            const std::chrono::high_resolution_clock::time_point tStepStart = std::chrono::high_resolution_clock::now();
            const SimTK::Integrator::SuccessfulStepStatus timestepRv = ts.stepTo(tNext.time_since_epoch().count());
            const std::chrono::high_resolution_clock::time_point tStepEnd = std::chrono::high_resolution_clock::now();

            // handle integrator response
            if (integ->isSimulationOver() &&
                integ->getTerminationReason() != SimTK::Integrator::ReachedFinalTime)
            {
                // simulation ended because of an error: report the error and exit
                log_error("%s", integ->getTerminationReasonString(integ->getTerminationReason()).c_str());
                return SimulationStatus::Error;
            }
            else if (timestepRv == SimTK::Integrator::ReachedReportTime)
            {
                // report the step and continue
                const std::chrono::duration<float> wallDur = tStepEnd - tSimStart;
                const std::chrono::duration<float> stepDur = tStepEnd - tStepStart;
                input.emitReport(CreateSimulationReport(wallDur, stepDur, input.getMultiBodySystem(), *integ));
                tLastReport = GetSimulationTime(*integ);
                ++step;
                continue;
            }
            else if (timestepRv == SimTK::Integrator::EndOfSimulation)
            {
                // if the simulation endpoint is sufficiently ahead of the last report time
                // (1 % of step size), then *also* report the simulation end time. Otherwise,
                // assume that there's an adjacent-enough report
                const SimulationClock::time_point t = GetSimulationTime(*integ);
                if ((tLastReport + 0.01*params.reportingInterval) < t)
                {
                    const std::chrono::duration<float> wallDur = tStepEnd - tSimStart;
                    const std::chrono::duration<float> stepDur = tStepEnd - tStepStart;
                    input.emitReport(CreateSimulationReport(wallDur, stepDur, input.getMultiBodySystem(), *integ));
                    tLastReport = t;
                }
                break;
            }
            else
            {
                // loop back and do the next timestep
            }
        }

        return SimulationStatus::Completed;
    }

    // MAIN function for the simulator thread
    //
    // guarded against exceptions (which are handled as simulation failures)
    int FdSimulationMain(
        cpp20::stop_token stopToken,
        std::unique_ptr<SimulatorThreadInput> input,
        std::shared_ptr<SharedState> shared)  // NOLINT(performance-unnecessary-value-param)
    {
        SimulationStatus status = SimulationStatus::Error;

        try
        {
            status = FdSimulationMainUnguarded(std::move(stopToken), *input, *shared);
        }
        catch (const OpenSim::Exception& ex)
        {
            log_error("OpenSim::Exception occurred when running a simulation: %s", ex.what());
        }
        catch (const std::exception& ex)
        {
            log_error("std::exception occurred when running a simulation: %s", ex.what());
        }
        catch (...)
        {
            log_error("an exception with unknown type occurred when running a simulation (no error message available)");
        }

        shared->setStatus(status);

        return 0;
    }
}

class osc::ForwardDynamicSimulator::Impl final {
public:
    Impl(BasicModelStatePair modelState,
        const ForwardDynamicSimulatorParams& params,
        std::function<void(SimulationReport)> onReportFromBgThread) :

        m_SimulationParams{params},
        m_Shared{std::make_shared<SharedState>()},
        m_SimulatorThread
        {
            FdSimulationMain,
            std::make_unique<SimulatorThreadInput>(std::move(modelState),
                                                   params,
                                                   std::move(onReportFromBgThread)),
            m_Shared
        }
    {
    }

    SimulationStatus getStatus() const
    {
        return m_Shared->getStatus();
    }

    void join()
    {
        if (m_SimulatorThread.joinable()) {
            m_SimulatorThread.join();
        }
    }

    void requestStop()
    {
        m_SimulatorThread.request_stop();
    }

    void stop()
    {
        m_SimulatorThread.request_stop();
        if (m_SimulatorThread.joinable()) {
            m_SimulatorThread.join();
        }
    }

    const ForwardDynamicSimulatorParams& params() const
    {
        return m_SimulationParams;
    }

private:
    ForwardDynamicSimulatorParams m_SimulationParams;
    std::shared_ptr<SharedState> m_Shared;
    cpp20::jthread m_SimulatorThread;
};


// public API

int osc::GetNumFdSimulatorOutputExtractors()
{
    return static_cast<int>(GetSimulatorOutputExtractors().size());
}

OutputExtractor osc::GetFdSimulatorOutputExtractor(int idx)
{
    return GetSimulatorOutputExtractors().at(static_cast<size_t>(idx));
}

osc::ForwardDynamicSimulator::ForwardDynamicSimulator(
    BasicModelStatePair msp,
    const ForwardDynamicSimulatorParams& params,
    std::function<void(SimulationReport)> onReportFromBgThread) :

    m_Impl{std::make_unique<Impl>(std::move(msp), params, std::move(onReportFromBgThread))}
{}
osc::ForwardDynamicSimulator::ForwardDynamicSimulator(ForwardDynamicSimulator&&) noexcept = default;
osc::ForwardDynamicSimulator& osc::ForwardDynamicSimulator::operator=(ForwardDynamicSimulator&&) noexcept = default;
osc::ForwardDynamicSimulator::~ForwardDynamicSimulator() noexcept = default;

SimulationStatus osc::ForwardDynamicSimulator::getStatus() const
{
    return m_Impl->getStatus();
}

void osc::ForwardDynamicSimulator::join()
{
    m_Impl->join();
}

void osc::ForwardDynamicSimulator::requestStop()
{
    m_Impl->requestStop();
}

void osc::ForwardDynamicSimulator::stop()
{
    m_Impl->stop();
}

const ForwardDynamicSimulatorParams& osc::ForwardDynamicSimulator::params() const
{
    return m_Impl->params();
}
