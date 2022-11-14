#include "ForwardDynamicSimulator.hpp"

#include "src/OpenSimBindings/BasicModelStatePair.hpp"
#include "src/OpenSimBindings/ForwardDynamicSimulatorParams.hpp"
#include "src/OpenSimBindings/IntegratorOutputExtractor.hpp"
#include "src/OpenSimBindings/IntegratorMethod.hpp"
#include "src/OpenSimBindings/MultiBodySystemOutputExtractor.hpp"
#include "src/OpenSimBindings/SimulationClock.hpp"
#include "src/OpenSimBindings/SimulationReport.hpp"
#include "src/OpenSimBindings/SimulationStatus.hpp"
#include "src/OpenSimBindings/VirtualOutputExtractor.hpp"
#include "src/Platform/Log.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/Cpp20Shims.hpp"
#include "src/Utils/UID.hpp"

#include <nonstd/span.hpp>
#include <OpenSim/Common/ComponentOutput.h>
#include <OpenSim/Common/Exception.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <SimTKsimbody.h>
#include <simmath/Integrator.h>
#include <simmath/TimeStepper.h>

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <exception>
#include <functional>
#include <memory>
#include <optional>
#include <ratio>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace OpenSim { class Component; }

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

    class AuxiliaryVariableOutputExtractor final : public osc::VirtualOutputExtractor {
    public:
        AuxiliaryVariableOutputExtractor(std::string name, std::string description, osc::UID uid) :
            m_Name{std::move(name)},
            m_Description{std::move(description)},
            m_UID{std::move(uid)}
        {
        }

        std::string const& getName() const override
        {
            return m_Name;
        }

        std::string const& getDescription() const override
        {
            return m_Description;
        }

        osc::OutputType getOutputType() const override
        {
            return osc::OutputType::Float;
        }

        float getValueFloat(OpenSim::Component const& c, osc::SimulationReport const& report) const override
        {
            nonstd::span<osc::SimulationReport const> reports(&report, 1);
            std::array<float, 1> out;
            getValuesFloat(c, reports, out);
            return out.front();
        }

        void getValuesFloat(OpenSim::Component const&,
                            nonstd::span<osc::SimulationReport const> reports,
                            nonstd::span<float> overwriteOut) const override
        {
            for (size_t i = 0; i < reports.size(); ++i)
            {
                overwriteOut[i] = reports[i].getAuxiliaryValue(m_UID).value_or(-1337.0f);
            }
        }

        std::string getValueString(OpenSim::Component const& c, osc::SimulationReport const& report) const override
        {
            return std::to_string(getValueFloat(c, report));
        }

        std::size_t getHash() const override
        {
            return osc::HashOf(m_Name, m_Description, m_UID);
        }

        bool equals(VirtualOutputExtractor const& other) const override
        {
            if (&other == this)
            {
                return true;
            }

            auto* otherT = dynamic_cast<AuxiliaryVariableOutputExtractor const*>(&other);

            if (!otherT)
            {
                return false;
            }

            bool result =
                m_Name == otherT->m_Name &&
                m_Description == otherT->m_Description &&
                m_UID == otherT->m_UID;

            return result;
        }

    private:
        std::string m_Name;
        std::string m_Description;
        osc::UID m_UID;
    };

    osc::UID const g_WalltimeUID;
    osc::UID const g_StepDurationUID;
}

static std::vector<osc::OutputExtractor> CreateSimulatorOutputExtractors()
{
    std::vector<osc::OutputExtractor> rv;
    rv.reserve(1 + osc::GetNumIntegratorOutputExtractors() + osc::GetNumMultiBodySystemOutputExtractors());

    {
        osc::OutputExtractor out{AuxiliaryVariableOutputExtractor{"Wall time", "Total cumulative time spent computing the simulation", g_WalltimeUID}};
        rv.push_back(out);

        osc::OutputExtractor out2{AuxiliaryVariableOutputExtractor{"Step Wall Time", "How long it took, in wall time, to compute the last integration step", g_StepDurationUID}};
        rv.push_back(out2);
    }

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
    integ->setReturnEveryInternalStep(true);  // so that cancellations/interrupts work
    integ->initialize(input.getState());
    return integ;
}

static osc::SimulationClock::time_point GetSimulationTime(SimTK::Integrator const& integ)
{
    return osc::SimulationClock::time_point(osc::SimulationClock::duration(integ.getTime()));
}

static osc::SimulationReport CreateSimulationReport(
    std::chrono::duration<float> wallTime,
    std::chrono::duration<float> stepDuration,
    SimTK::MultibodySystem const& sys,
    SimTK::Integrator const& integrator)
{
    SimTK::State st = integrator.getState();
    std::unordered_map<osc::UID, float> auxValues;

    // care: state needs to be realized on the simulator thread
    st.invalidateAllCacheAtOrAbove(SimTK::Stage::Instance);

    // populate forward dynamic simulator outputs
    {
        auxValues.emplace(g_WalltimeUID, wallTime.count());
        auxValues.emplace(g_StepDurationUID, stepDuration.count());
    }

    // populate integrator outputs
    {
        int numOutputs = osc::GetNumIntegratorOutputExtractors();
        auxValues.reserve(auxValues.size() + numOutputs);
        for (int i = 0; i < numOutputs; ++i)
        {
            osc::IntegratorOutputExtractor const& o = osc::GetIntegratorOutputExtractor(i);
            auxValues.emplace(o.getAuxiliaryDataID(), o.getExtractorFunction()(integrator));
        }
    }

    // populate mbs outputs
    {
        int numOutputs = osc::GetNumMultiBodySystemOutputExtractors();
        auxValues.reserve(auxValues.size() + numOutputs);
        for (int i = 0; i < numOutputs; ++i)
        {
            osc::MultiBodySystemOutputExtractor const& o = osc::GetMultiBodySystemOutputExtractor(i);
            auxValues.emplace(o.getAuxiliaryDataID(), o.getExtractorFunction()(sys));
        }
    }

    return osc::SimulationReport{std::move(st), std::move(auxValues)};
}

// this is the main function that the simulator thread works through (unguarded against exceptions)
static osc::SimulationStatus FdSimulationMainUnguarded(osc::stop_token stopToken,
                                                       SimulatorThreadInput& input,
                                                       SharedState& shared)
{
    std::chrono::high_resolution_clock::time_point const tSimStart = std::chrono::high_resolution_clock::now();

    osc::ForwardDynamicSimulatorParams const& params = input.getParams();

    // create + init an integrator
    std::unique_ptr<SimTK::Integrator> integ = CreateInitializedIntegrator(input);

    // create + init a timestepper for the integrator
    SimTK::TimeStepper ts{input.getMultiBodySystem(), *integ};
    ts.initialize(integ->getState());
    ts.setReportAllSignificantStates(true);  // so that cancellations/interrupts work

    // inform observers that everything has been initialized and the sim is now running
    shared.setStatus(osc::SimulationStatus::Running);

    // immediately report t = start
    {
        std::chrono::duration<float> wallDur = std::chrono::high_resolution_clock::now() - tSimStart;
        input.emitReport(CreateSimulationReport(wallDur, {}, input.getMultiBodySystem(), *integ));
    }

    // integrate (t0..tfinal]
    osc::SimulationClock::time_point tStart = GetSimulationTime(*integ);
    osc::SimulationClock::time_point tLastReport = tStart;
    int step = 1;
    while (!integ->isSimulationOver())
    {
        // check for cancellation requests
        if (stopToken.stop_requested())
        {
            return osc::SimulationStatus::Cancelled;
        }

        // calculate next reporting time
        osc::SimulationClock::time_point tNext = tStart + step*params.ReportingInterval;

        // perform an integration step
        std::chrono::high_resolution_clock::time_point tStepStart = std::chrono::high_resolution_clock::now();
        SimTK::Integrator::SuccessfulStepStatus timestepRv = ts.stepTo(tNext.time_since_epoch().count());
        std::chrono::high_resolution_clock::time_point tStepEnd = std::chrono::high_resolution_clock::now();

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
            std::chrono::duration<float> wallDur = tStepEnd - tSimStart;
            std::chrono::duration<float> stepDur = tStepEnd - tStepStart;
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
            osc::SimulationClock::time_point t = GetSimulationTime(*integ);
            if ((tLastReport + 0.01*params.ReportingInterval) < t)
            {
                std::chrono::duration<float> wallDur = tStepEnd - tSimStart;
                std::chrono::duration<float> stepDur = tStepEnd - tStepStart;
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
