#pragma once

#include "src/ui/AddBodyPopup.hpp"
#include "src/ui/AddComponentPopup.hpp"
#include "src/ui/select_2_pfs_popup.hpp"

#include <functional>
#include <optional>

namespace OpenSim {
    class Model;
    class Component;
}

namespace osc::ui::model_actions {
    struct State final {
        AddBodyPopup abm;
        ui::select_2_pfs::State select_2_pfs;
        int joint_idx_for_pfs_popup = -1;

        char const* add_component_popup_name = nullptr;
        std::optional<AddComponentPopup> add_component_popup = std::nullopt;

        State();
    };

    void draw(
        State&,
        OpenSim::Model& model,
        std::function<void(OpenSim::Component*)> const& on_set_selection,
        std::function<void()> const& on_before_modify_model,
        std::function<void()> const& on_after_modify_model);
}
