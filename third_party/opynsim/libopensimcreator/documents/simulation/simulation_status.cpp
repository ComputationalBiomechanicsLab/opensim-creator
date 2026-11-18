#include "simulation_status.h"

#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/enum_helpers.h>

#include <array>
#include <span>

using namespace osc;

namespace
{
    constexpr auto c_simulator_statuses = std::to_array<SimulationStatus>({
        SimulationStatus::Initializing,
        SimulationStatus::Running,
        SimulationStatus::Completed,
        SimulationStatus::Cancelled,
        SimulationStatus::Error,
    });
    static_assert(c_simulator_statuses.size() == num_options_v<SimulationStatus>);

    constexpr auto c_simulator_status_strings = std::to_array<CStringView>({
        "Initializing",
        "Running",
        "Completed",
        "Cancelled",
        "Error",
    });
    static_assert(c_simulator_status_strings.size() == num_options_v<SimulationStatus>);
}

std::span<const SimulationStatus> osc::get_all_simulation_statuses()
{
    return c_simulator_statuses;
}

std::span<const CStringView> osc::get_all_simulation_status_strings()
{
    return c_simulator_status_strings;
}
