#pragma once

#include <liboscar/utilities/c_string_view.h>

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

    std::span<const SimulationStatus> get_all_simulation_statuses();
    std::span<const CStringView> get_all_simulation_status_strings();
}
