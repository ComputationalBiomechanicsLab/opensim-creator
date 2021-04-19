#pragma once

#include <array>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace OpenSim {
    class Mesh;
}

namespace osc {
    std::vector<std::filesystem::path> find_all_vtp_resources();

    struct Attach_geometry_modal_state final {
        std::vector<std::filesystem::path> vtps = find_all_vtp_resources();
        std::vector<std::filesystem::path> recent_user_choices = {};
        std::array<char, 128> search = {};
    };

    // this assumes caller handles calling ImGui::OpenPopup(modal_name)

    void draw_attach_geom_modal_if_opened(
        Attach_geometry_modal_state&,
        char const* modal_name,
        std::function<void(std::unique_ptr<OpenSim::Mesh>)> const& out);
}
