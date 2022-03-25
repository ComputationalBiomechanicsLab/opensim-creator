#pragma once

#include <nonstd/span.hpp>

namespace osc
{
    enum class SimulationStatus
    {
        Initializing = 0,
        Running,
        Completed,
        Cancelled,
        Error,
        TOTAL,
    };
    nonstd::span<SimulationStatus const> GetAllSimulationStatuses();
    nonstd::span<char const* const> GetAllSimulationStatusStrings();
}
