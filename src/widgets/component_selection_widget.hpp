#pragma once

#include <functional>

namespace SimTK {
    class State;
}

namespace OpenSim {
    class Component;
}

namespace osmv {
    void draw_component_selection_widget(
        SimTK::State const& state,
        OpenSim::Component const* current_selection,
        std::function<void(OpenSim::Component const*)> const& on_selection_changed);
}
