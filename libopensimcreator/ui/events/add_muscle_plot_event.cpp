#include "add_muscle_plot_event.h"

#include <libopynsim/utilities/open_sim_helpers.h>
#include <OpenSim/Simulation/SimbodyEngine/Coordinate.h>
#include <OpenSim/Simulation/Model/Muscle.h>

osc::AddMusclePlotEvent::AddMusclePlotEvent(
    const OpenSim::Coordinate& coordinate,
    const OpenSim::Muscle& muscle) :

    m_CoordinateAbsPath{opyn::GetAbsolutePath(coordinate)},
    m_MuscleAbsPath{opyn::GetAbsolutePath(muscle)}
{
    enable_propagation();
}
