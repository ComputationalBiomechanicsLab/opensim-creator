#include "SimulationStatus.h"

#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/EnumHelpers.h>

#include <array>
#include <span>

using namespace osc;

namespace
{
    constexpr auto c_SimulatorStatuses = std::to_array<SimulationStatus>(
    {
        SimulationStatus::Initializing,
        SimulationStatus::Running,
        SimulationStatus::Completed,
        SimulationStatus::Cancelled,
        SimulationStatus::Error,
    });
    static_assert(c_SimulatorStatuses.size() == num_options<SimulationStatus>());

    constexpr auto c_SimulatorStatusStrings = std::to_array<CStringView>(
    {
        "Initializing",
        "Running",
        "Completed",
        "Cancelled",
        "Error",
    });
    static_assert(c_SimulatorStatusStrings.size() == num_options<SimulationStatus>());
}


// public API

std::span<const SimulationStatus> osc::GetAllSimulationStatuses()
{
    return c_SimulatorStatuses;
}

std::span<const CStringView> osc::GetAllSimulationStatusStrings()
{
    return c_SimulatorStatusStrings;
}
