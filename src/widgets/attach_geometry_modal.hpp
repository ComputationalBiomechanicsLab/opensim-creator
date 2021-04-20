#pragma once

#include <array>
#include <filesystem>
#include <memory>
#include <vector>

namespace OpenSim {
    class Mesh;
}

namespace osc {
    std::vector<std::filesystem::path> find_all_vtp_resources();
}

namespace osc::widgets::attach_geometry {
    struct State final {
        std::vector<std::filesystem::path> vtps = find_all_vtp_resources();
        std::vector<std::filesystem::path> recent_user_choices = {};
        std::array<char, 128> search = {};
    };

    // this assumes caller handles calling ImGui::OpenPopup(modal_name)
    //
    // returns non-nullptr when user selects an OpenSim::Mesh
    std::unique_ptr<OpenSim::Mesh> draw(State&, char const* modal_name);
}
