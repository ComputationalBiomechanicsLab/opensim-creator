#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <unordered_set>

namespace OpenSim { class Component; }

namespace osc
{
    // parameters used to create a "choose components" layer
    struct ChooseComponentsEditorLayerParameters final {

        // text that should be shown at the top of the layer
        std::string popupHeaderText = "Choose Something";

        // predicate that is used to test whether the element is choose-able
        std::function<bool(const OpenSim::Component&)> canChooseItem = [](const OpenSim::Component&)
        {
            return true;
        };

        // (maybe) the absolute paths of components that the user has already chosen, or is
        // assigning to (and, therefore, should maybe be highlighted but non-selectable)
        std::unordered_set<std::string> componentsBeingAssignedTo;

        size_t numComponentsUserMustChoose = 1;

        std::function<bool(const std::unordered_set<std::string>&)> onUserFinishedChoosing = [](const std::unordered_set<std::string>&)
        {
            return true;
        };
    };
}
