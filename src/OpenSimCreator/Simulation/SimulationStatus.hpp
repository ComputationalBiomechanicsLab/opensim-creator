#pragma once

#include <nonstd/span.hpp>

namespace osc
{
    // current status of an `osc::VirutalSimulation`
    enum class SimulationStatus
    {
        Initializing = 0,
        Running,
        Completed,
        Cancelled,
        Error,
        NUM_OPTIONS,
    };

    nonstd::span<SimulationStatus const> GetAllSimulationStatuses();
    nonstd::span<char const* const> GetAllSimulationStatusStrings();
}
