#include "SimulationStatus.hpp"

#include <oscar/Utils/Cpp20Shims.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/EnumHelpers.hpp>

#include <nonstd/span.hpp>

#include <cstddef>
#include <array>

namespace
{
    constexpr auto c_SimulatorStatuses = osc::to_array<osc::SimulationStatus>(
    {
        osc::SimulationStatus::Initializing,
        osc::SimulationStatus::Running,
        osc::SimulationStatus::Completed,
        osc::SimulationStatus::Cancelled,
        osc::SimulationStatus::Error,
    });
    static_assert(c_SimulatorStatuses.size() == osc::NumOptions<osc::SimulationStatus>());

    constexpr auto c_SimulatorStatusStrings = osc::to_array<osc::CStringView>(
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

nonstd::span<osc::CStringView const> osc::GetAllSimulationStatusStrings()
{
    return c_SimulatorStatusStrings;
}
