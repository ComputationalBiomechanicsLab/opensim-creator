#pragma once

#include "src/widgets/add_body_modal.hpp"
#include "src/widgets/add_component_popup.hpp"
#include "src/widgets/select_2_pfs_modal.hpp"

#include <functional>
#include <optional>

namespace OpenSim {
    class Model;
    class Component;
}

namespace osc::widgets::model_actions {
    struct State final {
        widgets::add_body::State abm;
        widgets::select_2_pfs::State select_2_pfs;
        int joint_idx_for_pfs_popup = -1;

        char const* add_component_popup_name = nullptr;
        std::optional<add_component::State> add_component_popup = std::nullopt;

        State();
    };

    void draw(
        State&,
        OpenSim::Model& model,
        std::function<void(OpenSim::Component*)> const& on_set_selection,
        std::function<void()> const& on_before_modify_model,
        std::function<void()> const& on_after_modify_model);
}
