#include "SimulationStatus.hpp"

#include <oscar/Utils/Cpp20Shims.hpp>
#include <oscar/Utils/EnumHelpers.hpp>

#include <nonstd/span.hpp>

#include <cstddef>
#include <array>

namespace
{
    auto constexpr c_SimulatorStatuses = osc::to_array<osc::SimulationStatus>(
    {
        osc::SimulationStatus::Initializing,
        osc::SimulationStatus::Running,
        osc::SimulationStatus::Completed,
        osc::SimulationStatus::Cancelled,
        osc::SimulationStatus::Error,
    });
    static_assert(c_SimulatorStatuses.size() == osc::NumOptions<osc::SimulationStatus>());

    auto constexpr c_SimulatorStatusStrings = osc::to_array<char const*>(
    {
        "Initializing",
        "Running",
        "Completed",
        "Cancelled",
        "Error",
    });
    static_assert(c_SimulatorStatusStrings.size() == osc::NumOptions<osc::SimulationStatus>());
}


// public API

nonstd::span<osc::SimulationStatus const> osc::GetAllSimulationStatuses()
{
    return c_SimulatorStatuses;
}

nonstd::span<char const* const> osc::GetAllSimulationStatusStrings()
{
    return c_SimulatorStatusStrings;
}
