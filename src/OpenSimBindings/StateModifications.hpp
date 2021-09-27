#pragma once

#include <string>
#include <unordered_map>

namespace OpenSim {
    class Coordinate;
    class Model;
}

namespace SimTK {
    class State;
}

namespace osc {
    // user-enacted coordinate edit
    //
    // used to modify the default state whenever a new state is generated
    struct CoordinateEdit final {
        double value;
        double speed;
        bool locked;

        bool applyToState(OpenSim::Coordinate const&, SimTK::State&) const;  // returns `true` if it modified the state
    };

    // user-enacted state modifications
    class StateModifications final {
    public:
        void pushCoordinateEdit(OpenSim::Coordinate const&, CoordinateEdit const&);
        bool applyToState(OpenSim::Model const&, SimTK::State&) const;

    private:
        std::unordered_map<std::string, CoordinateEdit> m_CoordEdits;
    };
}
