#pragma once

#include <Simbody.h>

namespace osc::fd
{
    /**
     * A convenience wrapper for the start and end points of an `Edge`
     */
    struct EdgePoints final {
        SimTK::Vec3 start{SimTK::NaN};
        SimTK::Vec3 end{SimTK::NaN};
    };
}
