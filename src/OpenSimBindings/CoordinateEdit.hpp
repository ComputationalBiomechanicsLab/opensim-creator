#pragma once

namespace OpenSim
{
    class Coordinate;
}

namespace SimTK
{
    class State;
}

namespace osc
{
    // a single, user-enacted, model coordinate edit
    //
    // used to modify the default state whenever a new state is generated
    struct CoordinateEdit final {
        double value;
        double speed;
        bool locked;

        // returns `true` if it modified the state
        bool applyToState(OpenSim::Coordinate const&, SimTK::State&) const;
    };
}