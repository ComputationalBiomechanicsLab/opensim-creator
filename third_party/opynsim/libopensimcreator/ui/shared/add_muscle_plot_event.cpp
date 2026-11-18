#include "add_muscle_plot_event.h"

#include <libopynsim/utilities/open_sim_helpers.h>
#include <OpenSim/Simulation/SimbodyEngine/Coordinate.h>
#include <OpenSim/Simulation/Model/Muscle.h>

osc::AddMusclePlotEvent::AddMusclePlotEvent(
    const OpenSim::Coordinate& coordinate, const OpenSim::Muscle& muscle) :

    coordinate_abs_path_{opyn::get_absolute_path(coordinate)},
    muscle_abs_path_{opyn::get_absolute_path(muscle)}
{
    enable_propagation();
}
