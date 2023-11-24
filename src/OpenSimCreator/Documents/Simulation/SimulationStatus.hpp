#pragma once

#include <oscar/Utils/CStringView.hpp>

#include <span>

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

    std::span<SimulationStatus const> GetAllSimulationStatuses();
    std::span<CStringView const> GetAllSimulationStatusStrings();
}
