#pragma once

#include <functional>
#include <string>

namespace OpenSim {
    class Model;
    class AbstractSocket;
    class Object;
}

namespace osmv {
    struct Reassign_socket_modal_state final {
        std::string error;
        char search[128]{};
    };

    // assumes caller handles ImGui::OpenPopup(modal_name);
    void draw_reassign_socket_modal(
        Reassign_socket_modal_state&,
        char const* modal_name,
        OpenSim::Model const&,
        OpenSim::AbstractSocket const&,
        std::function<void(OpenSim::Object const&)> const& on_conectee_change_request);
}
