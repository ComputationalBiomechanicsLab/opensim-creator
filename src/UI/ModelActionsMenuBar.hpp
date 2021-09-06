#pragma once

#include "src/UI/AddBodyPopup.hpp"
#include "src/UI/AddComponentPopup.hpp"
#include "src/UI/Select2PFsPopup.hpp"

#include <functional>
#include <optional>

namespace OpenSim {
    class Model;
    class Component;
}

namespace osc {
    struct ModelActionsMenuBar final {
        AddBodyPopup abm;
        Select2PFsPopup select2PFsPopup;
        int jointIndexForPFsPopup;
        char const* addComponentPopupName;
        std::optional<AddComponentPopup> addComponentPopup;

        ModelActionsMenuBar();

        void draw(
            OpenSim::Model& model,
            std::function<void(OpenSim::Component*)> const& onSetSelection,
            std::function<void()> const& onBeforeModifyModel,
            std::function<void()> const& onAfterModifyModel);
    };
}
