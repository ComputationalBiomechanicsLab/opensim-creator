#pragma once

#include <functional>
#include <vector>

namespace OpenSim {
    class Object;
    class AbstractProperty;
}

namespace osmv {
    struct Property_editor_state final {
        bool is_locked = true;
    };

    struct Properties_editor_state final {
        std::vector<Property_editor_state> property_editor_states;
    };

    void draw_properties_editor(
        Properties_editor_state&,
        OpenSim::Object&,
        std::function<void()> const& before_property_edited,
        std::function<void()> const& after_property_edited);

    void draw_properties_editor_for_props_with_indices(
        Properties_editor_state&,
        OpenSim::Object&,
        int* indices,
        size_t nindices,
        std::function<void()> const& before_property_edited,
        std::function<void()> const& after_property_edited);
}
