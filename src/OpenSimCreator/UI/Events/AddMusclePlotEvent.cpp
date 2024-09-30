#include "AddMusclePlotEvent.h"

#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <OpenSim/Simulation/SimbodyEngine/Coordinate.h>
#include <OpenSim/Simulation/Model/Muscle.h>

osc::AddMusclePlotEvent::AddMusclePlotEvent(
    const OpenSim::Coordinate& coordinate,
    const OpenSim::Muscle& muscle) :

    m_CoordinateAbsPath{GetAbsolutePath(coordinate)},
    m_MuscleAbsPath{GetAbsolutePath(muscle)}
{
    enable_propagation();
}
