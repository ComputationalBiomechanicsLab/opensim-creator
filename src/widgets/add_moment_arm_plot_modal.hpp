#pragma once

#include <functional>
#include <utility>
#include <vector>

namespace OpenSim {
    class Muscle;
    class Coordinate;
    class Model;
}

namespace osc {
    struct Add_moment_arm_plot_modal_state final {
        std::vector<OpenSim::Muscle const*> muscles_scratch;
        std::vector<OpenSim::Coordinate const*> coords_scratch;
        OpenSim::Muscle const* selected_muscle = nullptr;
        OpenSim::Coordinate const* selected_coord = nullptr;
    };

    // assumes caller has handled calling ImGui::OpenPopup(modal_name)
    void draw_add_moment_arm_plot_modal(
        Add_moment_arm_plot_modal_state&,
        char const* modal_name,
        OpenSim::Model const&,
        std::function<void(std::pair<OpenSim::Muscle const*, OpenSim::Coordinate const*>)> const&
            on_add_plot_requested);
}
