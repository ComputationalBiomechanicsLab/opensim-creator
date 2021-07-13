#pragma once

#include <optional>

namespace OpenSim {
    class PhysicalFrame;
    class Model;
}

namespace osc::ui::select_2_pfs {

    struct State final {
        OpenSim::PhysicalFrame const* first = nullptr;
        OpenSim::PhysicalFrame const* second = nullptr;
    };

    struct Response final {
        OpenSim::PhysicalFrame const& first;
        OpenSim::PhysicalFrame const& second;

        Response(OpenSim::PhysicalFrame const& _first, OpenSim::PhysicalFrame const& _second) :
            first{_first},
            second{_second} {
        }
    };

    // assumes caller has handled calling ImGui::OpenPopup(modal_name)
    std::optional<Response> draw(
            State&, char
            const* modal_name,
            OpenSim::Model const&,
            char const* first_label,
            char const* second_label);
}
