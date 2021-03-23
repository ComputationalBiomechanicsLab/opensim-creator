#pragma once

#include "src/widgets/add_body_modal.hpp"
#include "src/widgets/add_joint_modal.hpp"
#include "src/widgets/select_2_pfs_modal.hpp"

#include <functional>

namespace OpenSim {
    class Model;
    class Component;
}

namespace osmv {
    struct Model_actions_panel_state final {
        Added_body_modal_state abm;
        std::array<Add_joint_modal, 4> add_joint_modals;
        Select_2_pfs_modal_state select_2_pfs;

        Model_actions_panel_state();
    };

    void draw_model_actions_panel(
        Model_actions_panel_state&,
        OpenSim::Model& model,
        std::function<void(OpenSim::Component*)> const& on_set_selection,
        std::function<void()> const& on_before_modify_model,
        std::function<void()> const& on_after_modify_model);
}
