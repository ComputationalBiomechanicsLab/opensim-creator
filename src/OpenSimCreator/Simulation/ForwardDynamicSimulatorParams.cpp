#include "ForwardDynamicSimulatorParams.hpp"

#include "OpenSimCreator/Simulation/SimulationClock.hpp"

#include <oscar/Utils/CStringView.hpp>

#include <chrono>
#include <memory>
#include <optional>
#include <variant>

namespace
{
    osc::CStringView constexpr c_FinalTimeTitle = "Final Time (sec)";
    osc::CStringView constexpr c_FinalTimeDesc = "The final time, in seconds, that the forward dynamic simulation should integrate up to";
    osc::CStringView constexpr c_IntegratorMethodUsedTitle = "Integrator Method";
    osc::CStringView constexpr c_IntegratorMethodUsedDesc = "The integrator that the forward dynamic simulator should use. OpenSim's default integrator is a good choice if you aren't familiar with the other integrators. Changing the integrator can have a large impact on the performance and accuracy of the simulation.";
    osc::CStringView constexpr c_ReportingIntervalTitle = "Reporting Interval (sec)";
    osc::CStringView constexpr c_ReportingIntervalDesc = "How often the simulator should emit a simulation report. This affects how many datapoints are collected for the animation, output values, etc.";
    osc::CStringView constexpr c_IntegratorStepLimitTitle = "Integrator Step Limit";
    osc::CStringView constexpr c_IntegratorStepLimitDesc = "The maximum number of *internal* steps that can be taken within a single call to the integrator's stepTo/stepBy function. This is mostly an internal engine concern, but can occasionally affect how often reports are emitted";
    osc::CStringView constexpr c_IntegratorMinimumStepSizeTitle = "Minimum Step Size (sec)";
    osc::CStringView constexpr c_IntegratorMinimumStepSizeDesc = "The minimum step size, in seconds, that the integrator must take during the simulation. Note: this is mostly only relevant for error-corrected integrators that change their step size dynamically as the simulation runs.";
    osc::CStringView constexpr c_IntegratorMaximumStepSizeTitle = "Maximum step size (sec)";
    osc::CStringView constexpr c_IntegratorMaximumStepSizeDesc = "The maximum step size, in seconds, that the integrator must take during the simulation. Note: this is mostly only relevant for error-correct integrators that change their step size dynamically as the simulation runs";
    osc::CStringView constexpr c_IntegratorAccuracyTitle = "Accuracy";
    osc::CStringView constexpr c_IntegratorAccuracyDesc = "Target accuracy for the integrator. Mostly only relevant for error-controlled integrators that change their step size by comparing this accuracy value to measured integration error";
}


// public API

osc::ForwardDynamicSimulatorParams::ForwardDynamicSimulatorParams() :
    finalTime{SimulationClock::start() + SimulationClock::duration{10.0}},
    integratorMethodUsed{IntegratorMethod::OpenSimManagerDefault},
    reportingInterval{1.0/100.0},
    integratorStepLimit{20000},
    integratorMinimumStepSize{1.0e-8},
    integratorMaximumStepSize{1.0},
    integratorAccuracy{1.0e-5}
{
}

bool osc::operator==(ForwardDynamicSimulatorParams const& a, ForwardDynamicSimulatorParams const& b)
{
    return
        a.finalTime == b.finalTime &&
        a.integratorMethodUsed == b.integratorMethodUsed &&
        a.reportingInterval == b.reportingInterval &&
        a.integratorStepLimit == b.integratorStepLimit &&
        a.integratorMinimumStepSize == b.integratorMinimumStepSize &&
        a.integratorMaximumStepSize == b.integratorMaximumStepSize &&
        a.integratorAccuracy == b.integratorAccuracy;
}

osc::ParamBlock osc::ToParamBlock(ForwardDynamicSimulatorParams const& p)
{
    ParamBlock rv;
    rv.pushParam(c_FinalTimeTitle, c_FinalTimeDesc, (p.finalTime - SimulationClock::start()).count());
    rv.pushParam(c_IntegratorMethodUsedTitle, c_IntegratorMethodUsedDesc, p.integratorMethodUsed);
    rv.pushParam(c_ReportingIntervalTitle, c_ReportingIntervalDesc, p.reportingInterval.count());
    rv.pushParam(c_IntegratorStepLimitTitle, c_IntegratorStepLimitDesc, p.integratorStepLimit);
    rv.pushParam(c_IntegratorMinimumStepSizeTitle, c_IntegratorMinimumStepSizeDesc, p.integratorMinimumStepSize.count());
    rv.pushParam(c_IntegratorMaximumStepSizeTitle, c_IntegratorMaximumStepSizeDesc, p.integratorMaximumStepSize.count());
    rv.pushParam(c_IntegratorAccuracyTitle, c_IntegratorAccuracyDesc, p.integratorAccuracy);
    return rv;
}

osc::ForwardDynamicSimulatorParams osc::FromParamBlock(ParamBlock const& b)
{
    ForwardDynamicSimulatorParams rv;
    if (auto finalTime = b.findValue(c_FinalTimeTitle); finalTime && std::holds_alternative<double>(*finalTime))
    {
        rv.finalTime = SimulationClock::start() + SimulationClock::duration{std::get<double>(*finalTime)};
    }
    if (auto integMethod = b.findValue(c_IntegratorMethodUsedTitle); integMethod && std::holds_alternative<IntegratorMethod>(*integMethod))
    {
        rv.integratorMethodUsed = std::get<IntegratorMethod>(*integMethod);
    }
    if (auto repInterv = b.findValue(c_ReportingIntervalTitle); repInterv && std::holds_alternative<double>(*repInterv))
    {
        rv.reportingInterval = SimulationClock::duration{std::get<double>(*repInterv)};
    }
    if (auto stepLim = b.findValue(c_IntegratorStepLimitTitle); stepLim && std::holds_alternative<int>(*stepLim))
    {
        rv.integratorStepLimit = std::get<int>(*stepLim);
    }
    if (auto minStep = b.findValue(c_IntegratorMinimumStepSizeTitle); minStep && std::holds_alternative<double>(*minStep))
    {
        rv.integratorMinimumStepSize = SimulationClock::duration{std::get<double>(*minStep)};
    }
    if (auto maxStep = b.findValue(c_IntegratorMaximumStepSizeTitle); maxStep && std::holds_alternative<double>(*maxStep))
    {
        rv.integratorMaximumStepSize = SimulationClock::duration{std::get<double>(*maxStep)};
    }
    if (auto acc = b.findValue(c_IntegratorAccuracyTitle); acc && std::holds_alternative<double>(*acc))
    {
        rv.integratorAccuracy = std::get<double>(*acc);
    }
    return rv;
}
