#pragma once

#include "src/UI/AddBodyPopup.hpp"
#include "src/UI/AddComponentPopup.hpp"
#include "src/UI/Select2PFsPopup.hpp"

#include <optional>

namespace OpenSim {
    class Model;
    class Component;
}

namespace osc {
    class UiModel;
}

namespace osc {
    struct ModelActionsMenuBar final {
        AddBodyPopup abm;
        Select2PFsPopup select2PFsPopup;
        int jointIndexForPFsPopup;
        char const* addComponentPopupName;
        std::optional<AddComponentPopup> addComponentPopup;

        ModelActionsMenuBar();

        bool draw(UiModel&);
    };
}
