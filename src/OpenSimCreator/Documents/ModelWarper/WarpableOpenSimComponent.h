#pragma once

#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/PhysicalOffsetFrame.h>
#include <oscar/Utils/Concepts.h>

namespace osc::mow
{
    template<typename T>
    concept WarpableOpenSimComponent = true;  // TODO
}
