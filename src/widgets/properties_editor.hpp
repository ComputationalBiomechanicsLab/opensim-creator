#pragma once

#include <functional>
#include <vector>

namespace OpenSim {
    class Component;
}

namespace osmv {
    struct Properties_editor_state final {
        std::vector<bool> property_locked;
    };

    void draw_properties_editor(
        Properties_editor_state&,
        OpenSim::Component&,
        std::function<void()> const& before_property_edited,
        std::function<void()> const& after_property_edited);
}
