#pragma once

#include <memory>

namespace OpenSim
{
    class Coordinate;
    class Model;
}

namespace SimTK
{
    class State;
}

namespace osc
{
    // user-enacted coordinate edit
    //
    // used to modify the default state whenever a new state is generated
    struct CoordinateEdit final {
        double value;
        double speed;
        bool locked;

        // returns `true` if it modified the state
        bool applyToState(OpenSim::Coordinate const&, SimTK::State&) const;
    };

    // user-enacted state modifications
    class StateModifications final {
    public:
        StateModifications();
        StateModifications(StateModifications const&);
        StateModifications(StateModifications&&) noexcept;
        ~StateModifications() noexcept;

        StateModifications& operator=(StateModifications const&);
        StateModifications& operator=(StateModifications&&) noexcept;

        void pushCoordinateEdit(OpenSim::Coordinate const&, CoordinateEdit const&);
        bool removeCoordinateEdit(OpenSim::Coordinate const&);
        bool applyToState(OpenSim::Model const&, SimTK::State&);

        class Impl;
    private:
        std::unique_ptr<Impl> m_Impl;
    };
}
