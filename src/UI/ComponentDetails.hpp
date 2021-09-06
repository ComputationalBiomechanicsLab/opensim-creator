#pragma once

namespace SimTK {
    class State;
}

namespace OpenSim {
    class Component;
}

namespace osc {

    class ComponentDetails final {
    public:
        enum ResponseType { NothingHappened, SelectionChanged };
        struct Response final {
            ResponseType type = NothingHappened;
            OpenSim::Component const* ptr = nullptr;
        };

        Response draw(SimTK::State const&, OpenSim::Component const*);
    };
}
