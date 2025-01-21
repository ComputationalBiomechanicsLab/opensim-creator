#pragma once

#include <liboscar/Utils/CStringView.h>

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

    std::span<const SimulationStatus> GetAllSimulationStatuses();
    std::span<const CStringView> GetAllSimulationStatusStrings();
}
