#include "SimulationStatus.hpp"

#include <nonstd/span.hpp>

#include <cstddef>
#include <array>


static std::array<osc::SimulationStatus, static_cast<size_t>(osc::SimulationStatus::TOTAL)> CreateSimulationStatusLut()
{
    return {
        osc::SimulationStatus::Initializing,
        osc::SimulationStatus::Running,
        osc::SimulationStatus::Completed,
        osc::SimulationStatus::Cancelled,
        osc::SimulationStatus::Error,
    };
}

static std::array<char const*, static_cast<size_t>(osc::SimulationStatus::TOTAL)> CreateSimulationStatusStrings()
{
    return {
        "Initializing",
        "Running",
        "Completed",
        "Cancelled",
        "Error",
    };
}

// public API

nonstd::span<osc::SimulationStatus const> osc::GetAllSimulationStatuses()
{
    static auto const g_SimulatorStatuses = CreateSimulationStatusLut();
    return g_SimulatorStatuses;
}

nonstd::span<char const* const> osc::GetAllSimulationStatusStrings()
{
    static auto const g_SimulatorStatusStrings = CreateSimulationStatusStrings();
    return g_SimulatorStatusStrings;
}
