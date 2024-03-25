#pragma once

#include <oscar/Utils/UID.h>

namespace osc
{
    // an OSC-specific value that is (usually) attached to a state of a simulation
    //
    // (e.g. it's used for attaching integrator metadata to a `SimTK::State`)
    struct AuxiliaryValue final {
        UID id;
        float value;
    };
}
