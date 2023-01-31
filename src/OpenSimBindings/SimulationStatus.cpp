#include "SimulationStatus.hpp"

#include "src/Utils/Algorithms.hpp"

#include <nonstd/span.hpp>

#include <cstddef>
#include <array>

static auto constexpr c_SimulatorStatuses = osc::MakeSizedArray<osc::SimulationStatus, static_cast<size_t>(osc::SimulationStatus::TOTAL)>
(
    osc::SimulationStatus::Initializing,
    osc::SimulationStatus::Running,
    osc::SimulationStatus::Completed,
    osc::SimulationStatus::Cancelled,
    osc::SimulationStatus::Error
);

static auto constexpr c_SimulatorStatusStrings = osc::MakeSizedArray<char const*, static_cast<size_t>(osc::SimulationStatus::TOTAL)>
(
    "Initializing",
    "Running",
    "Completed",
    "Cancelled",
    "Error"
);


// public API

nonstd::span<osc::SimulationStatus const> osc::GetAllSimulationStatuses()
{
    return c_SimulatorStatuses;
}

nonstd::span<char const* const> osc::GetAllSimulationStatusStrings()
{
    return c_SimulatorStatusStrings;
}
