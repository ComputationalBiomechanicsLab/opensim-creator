#pragma once

#include <liboscar/utilities/string_name.h>

#include <cstddef>
#include <functional>
#include <string>
#include <unordered_set>

namespace OpenSim
{
    class Component;
}

namespace osc
{
    // parameters used to create a "choose components" layer
    struct ChooseComponentsEditorLayerParameters final {

        // text that should be shown at the top of the layer
        std::string popup_header_text = "Choose Something";

        // predicate that is used to test whether the element is choose-able
        std::function<bool(const OpenSim::Component&)> can_choose_item =
            [](const OpenSim::Component&) {
                return true;
            };

        // (maybe) the absolute paths of components that the user has already chosen, or is
        // assigning to (and, therefore, should maybe be highlighted but non-selectable)
        std::unordered_set<StringName> components_being_assigned_to;

        size_t num_components_user_must_choose = 1;

        std::function<bool(const std::unordered_set<StringName>&)> on_user_finished_choosing =
            [](const std::unordered_set<StringName>&) {
                return true;
            };
    };
}
