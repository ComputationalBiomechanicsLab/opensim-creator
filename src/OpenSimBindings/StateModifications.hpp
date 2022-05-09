#pragma once

#include "src/OpenSimBindings/CoordinateEdit.hpp"

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
    // a collection of user-enacted state modifications
    class StateModifications final {
    public:
        StateModifications();
        StateModifications(StateModifications const&);
        StateModifications(StateModifications&&) noexcept;
        StateModifications& operator=(StateModifications const&);
        StateModifications& operator=(StateModifications&&) noexcept;
        ~StateModifications() noexcept;

        void pushCoordinateEdit(OpenSim::Coordinate const&, CoordinateEdit const&);
        bool removeCoordinateEdit(OpenSim::Coordinate const&);
        bool applyToState(OpenSim::Model const&, SimTK::State&);

        class Impl;
    private:
        std::unique_ptr<Impl> m_Impl;
    };
}
