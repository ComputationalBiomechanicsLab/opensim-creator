#pragma once

#include <functional>

namespace OpenSim {
    class PhysicalFrame;
    class Model;
}

namespace osc {

    struct Select_2_pfs_modal_state final {
        OpenSim::PhysicalFrame const* first = nullptr;
        OpenSim::PhysicalFrame const* second = nullptr;
    };

    struct Select_2_pfs_modal_output final {
        OpenSim::PhysicalFrame const& first;
        OpenSim::PhysicalFrame const& second;
    };

    // assumes caller has handled calling ImGui::OpenPopup(modal_name)
    void draw_select_2_pfs_modal(
        Select_2_pfs_modal_state&,
        char const* modal_name,
        OpenSim::Model const&,
        char const* first_label,
        char const* second_label,
        std::function<void(Select_2_pfs_modal_output)> const&);
}
