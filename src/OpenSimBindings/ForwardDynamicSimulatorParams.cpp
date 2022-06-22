#include "ForwardDynamicSimulatorParams.hpp"

#include "src/OpenSimBindings/SimulationClock.hpp"

#include <chrono>
#include <memory>
#include <optional>
#include <variant>

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


// public API

osc::ForwardDynamicSimulatorParams::ForwardDynamicSimulatorParams() :
    FinalTime{SimulationClock::start() + SimulationClock::duration{10.0}},
    IntegratorMethodUsed{IntegratorMethod::OpenSimManagerDefault},
    ReportingInterval{1.0/100.0},
    IntegratorStepLimit{20000},
    IntegratorMinimumStepSize{1.0e-8},
    IntegratorMaximumStepSize{1.0},
    IntegratorAccuracy{1.0e-5}
{
}

osc::ParamBlock osc::ToParamBlock(ForwardDynamicSimulatorParams const& p)
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

osc::ForwardDynamicSimulatorParams osc::FromParamBlock(ParamBlock const& b)
{
    ForwardDynamicSimulatorParams rv;
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
