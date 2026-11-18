#include "forward_dynamic_simulator_params.h"

#include <libopensimcreator/documents/simulation/simulation_clock.h>

#include <liboscar/utilities/c_string_view.h>

#include <chrono>
#include <optional>
#include <variant>

using namespace osc;

namespace
{
    constexpr CStringView c_final_time_title = "Final Time (sec)";
    constexpr CStringView c_final_time_desc =
        "The final time, in seconds, that the forward dynamic simulation should integrate up to";
    constexpr CStringView c_integrator_method_used_title = "Integrator Method";
    constexpr CStringView c_integrator_method_used_desc =
        "The integrator that the forward dynamic simulator should use. OpenSim's default integrator is a good choice if you aren't familiar with the other integrators. Changing the integrator can have a large impact on the performance and accuracy of the simulation.";
    constexpr CStringView c_reporting_interval_title = "Reporting Interval (sec)";
    constexpr CStringView c_reporting_interval_desc =
        "How often the simulator should emit a simulation report. This affects how many datapoints are collected for the animation, output values, etc.";
    constexpr CStringView c_integrator_step_limit_title = "Integrator Step Limit";
    constexpr CStringView c_integrator_step_limit_desc =
        "The maximum number of *internal* steps that can be taken within a single call to the integrator's stepTo/stepBy function. This is mostly an internal engine concern, but can occasionally affect how often reports are emitted";
    constexpr CStringView c_integrator_minimum_step_size_title = "Minimum Step Size (sec)";
    constexpr CStringView c_integrator_minimum_step_size_desc =
        "The minimum step size, in seconds, that the integrator must take during the simulation. Note: this is mostly only relevant for error-corrected integrators that change their step size dynamically as the simulation runs.";
    constexpr CStringView c_integrator_maximum_step_size_title = "Maximum step size (sec)";
    constexpr CStringView c_integrator_maximum_step_size_desc =
        "The maximum step size, in seconds, that the integrator must take during the simulation. Note: this is mostly only relevant for error-correct integrators that change their step size dynamically as the simulation runs";
    constexpr CStringView c_integrator_accuracy_title = "Accuracy";
    constexpr CStringView c_integrator_accuracy_desc =
        "Target accuracy for the integrator. Mostly only relevant for error-controlled integrators that change their step size by comparing this accuracy value to measured integration error";
}

ParamBlock osc::to_param_block(const ForwardDynamicSimulatorParams& p)
{
    ParamBlock rv;
    rv.push_param(
        c_final_time_title, c_final_time_desc, (p.final_time - SimulationClock::start()).count());
    rv.push_param(
        c_integrator_method_used_title, c_integrator_method_used_desc, p.integrator_method_used);
    rv.push_param(
        c_reporting_interval_title, c_reporting_interval_desc, p.reporting_interval.count());
    rv.push_param(
        c_integrator_step_limit_title, c_integrator_step_limit_desc, p.integrator_step_limit);
    rv.push_param(
        c_integrator_minimum_step_size_title,
        c_integrator_minimum_step_size_desc,
        p.integrator_minimum_step_size.count());
    rv.push_param(
        c_integrator_maximum_step_size_title,
        c_integrator_maximum_step_size_desc,
        p.integrator_maximum_step_size.count());
    rv.push_param(c_integrator_accuracy_title, c_integrator_accuracy_desc, p.integrator_accuracy);
    return rv;
}

ForwardDynamicSimulatorParams osc::from_param_block(const ParamBlock& b)
{
    ForwardDynamicSimulatorParams rv;
    if (auto final_time = b.find_value(c_final_time_title);
        final_time && std::holds_alternative<double>(*final_time)) {
        rv.final_time =
            SimulationClock::start() + SimulationClock::duration{std::get<double>(*final_time)};
    }
    if (auto integ_method = b.find_value(c_integrator_method_used_title);
        integ_method && std::holds_alternative<opyn::IntegratorMethod>(*integ_method)) {
        rv.integrator_method_used = std::get<opyn::IntegratorMethod>(*integ_method);
    }
    if (auto rep_interv = b.find_value(c_reporting_interval_title);
        rep_interv && std::holds_alternative<double>(*rep_interv)) {
        rv.reporting_interval = SimulationClock::duration{std::get<double>(*rep_interv)};
    }
    if (auto step_lim = b.find_value(c_integrator_step_limit_title);
        step_lim && std::holds_alternative<int>(*step_lim)) {
        rv.integrator_step_limit = std::get<int>(*step_lim);
    }
    if (auto min_step = b.find_value(c_integrator_minimum_step_size_title);
        min_step && std::holds_alternative<double>(*min_step)) {
        rv.integrator_minimum_step_size = SimulationClock::duration{std::get<double>(*min_step)};
    }
    if (auto max_step = b.find_value(c_integrator_maximum_step_size_title);
        max_step && std::holds_alternative<double>(*max_step)) {
        rv.integrator_maximum_step_size = SimulationClock::duration{std::get<double>(*max_step)};
    }
    if (auto acc = b.find_value(c_integrator_accuracy_title);
        acc && std::holds_alternative<double>(*acc)) {
        rv.integrator_accuracy = std::get<double>(*acc);
    }
    return rv;
}
