#pragma once

#include "src/Utils/SimTKHelpers.hpp"

namespace OpenSim
{
    class Component;
}

namespace osc
{
    struct ComponentDecoration : public SystemDecoration {
        OpenSim::Component const* component;

        ComponentDecoration(SystemDecoration const& se,
                             OpenSim::Component const* c) :
            SystemDecoration{se},
            component{c}
        {
        }
    };
}
