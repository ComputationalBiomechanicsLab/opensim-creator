#pragma once

#include <liboscar/Utils/Concepts.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/PhysicalOffsetFrame.h>
#include <OpenSim/Simulation/Model/Station.h>

namespace osc::mow
{
    // satisfied by OpenSim types that can be warped by the model warper
    template<typename T>
    concept WarpableOpenSimComponent = IsAnyOf<T,
        OpenSim::Mesh,
        OpenSim::PhysicalOffsetFrame,
        OpenSim::Station
    >;
}
