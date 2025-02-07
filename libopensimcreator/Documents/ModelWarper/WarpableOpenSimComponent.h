#pragma once

#include <liboscar/Concepts/SameAsAnyOf.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/PhysicalOffsetFrame.h>
#include <OpenSim/Simulation/Model/Station.h>

namespace osc::mow
{
    // satisfied by OpenSim types that can be warped by the model warper
    template<typename T>
    concept WarpableOpenSimComponent = SameAsAnyOf<T,
        OpenSim::Mesh,
        OpenSim::PhysicalOffsetFrame,
        OpenSim::Station
    >;
}
