#include "SimulationStatus.hpp"

#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/EnumHelpers.hpp>

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
    static_assert(c_SimulatorStatuses.size() == NumOptions<SimulationStatus>());

    constexpr auto c_SimulatorStatusStrings = std::to_array<CStringView>(
    {
        "Initializing",
        "Running",
        "Completed",
        "Cancelled",
        "Error",
    });
    static_assert(c_SimulatorStatusStrings.size() == NumOptions<SimulationStatus>());
}


// public API

std::span<SimulationStatus const> osc::GetAllSimulationStatuses()
{
    return c_SimulatorStatuses;
}

std::span<CStringView const> osc::GetAllSimulationStatusStrings()
{
    return c_SimulatorStatusStrings;
}
