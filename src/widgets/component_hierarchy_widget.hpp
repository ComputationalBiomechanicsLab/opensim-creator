#pragma once

#include <functional>

namespace OpenSim {
    class Component;
}

namespace osc {
    void draw_component_hierarchy_widget(
        OpenSim::Component const* root,
        OpenSim::Component const* current_selection,
        OpenSim::Component const* current_hover,
        std::function<void(OpenSim::Component const*)> const& on_selection_change,
        std::function<void(OpenSim::Component const*)> const& on_hover_changed);
}
