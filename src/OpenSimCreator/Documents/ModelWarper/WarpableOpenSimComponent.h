#pragma once

#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/PhysicalOffsetFrame.h>
#include <oscar/Utils/Concepts.h>

namespace osc::mow
{
    // a compile-time list of OpenSim types that are specifically handled by the model warper
    template<typename T>
    concept WarpableOpenSimComponent = IsAnyOf<T,
        OpenSim::Mesh,
        OpenSim::PhysicalOffsetFrame
    >;
}
