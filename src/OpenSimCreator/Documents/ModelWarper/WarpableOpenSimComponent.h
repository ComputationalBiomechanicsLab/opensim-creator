#pragma once

#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/PhysicalOffsetFrame.h>
#include <OpenSim/Simulation/Model/Station.h>
#include <oscar/Utils/Concepts.h>

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
