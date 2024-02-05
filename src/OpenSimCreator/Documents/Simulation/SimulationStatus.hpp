#pragma once

#include <oscar/Utils/CStringView.hpp>

#include <span>

namespace osc
{
    // current status of an `VirutalSimulation`
    enum class SimulationStatus {
        Initializing,
        Running,
        Completed,
        Cancelled,
        Error,
        NUM_OPTIONS,
    };

    std::span<SimulationStatus const> GetAllSimulationStatuses();
    std::span<CStringView const> GetAllSimulationStatusStrings();
}
