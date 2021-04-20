#pragma once

namespace SimTK {
    class State;
}

namespace OpenSim {
    class Component;
}

namespace osc::widgets::component_details {
    enum Response_type {
        NothingHappened,
        SelectionChanged
    };

    struct Response final {
        Response_type type = NothingHappened;
        OpenSim::Component const* ptr = nullptr;
    };

    Response draw(
        SimTK::State const& state,
        OpenSim::Component const* current_selection);
}
