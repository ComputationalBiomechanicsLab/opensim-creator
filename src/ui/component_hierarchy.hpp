#pragma once

namespace OpenSim {
    class Component;
}

namespace osc::widgets::component_hierarchy {
    enum Response_type { NothingHappened, SelectionChanged, HoverChanged };

    struct Response final {
        OpenSim::Component const* ptr = nullptr;
        Response_type type = NothingHappened;
    };

    Response draw(
        OpenSim::Component const* root,
        OpenSim::Component const* current_selection,
        OpenSim::Component const* current_hover);
}
