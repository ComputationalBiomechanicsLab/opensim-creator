#pragma once

#include <array>
#include <filesystem>
#include <memory>
#include <vector>

namespace OpenSim {
    class Geometry;
}

namespace osc::ui::attach_geometry_popup {
    struct State final {
        // vtps found in the user's/installation's `Geometry/` dir
        std::vector<std::filesystem::path> vtps;

        // recent file choices by the user
        std::vector<std::filesystem::path> recent_user_choices;

        // C string that stores the user's current search term
        std::array<char, 128> search;

        State();
    };

    // this assumes caller handles calling ImGui::OpenPopup(modal_name)
    //
    // returns non-nullptr when user selects an OpenSim::Mesh
    std::unique_ptr<OpenSim::Geometry> draw(State&, char const* modal_name);
}
