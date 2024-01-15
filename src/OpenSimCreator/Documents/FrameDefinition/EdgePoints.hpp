#pragma once

#include <Simbody.h>

namespace osc::fd
{
    // the start and end locations of an edge in 3D space
    struct EdgePoints final {
        SimTK::Vec3 start;
        SimTK::Vec3 end;
    };
}
