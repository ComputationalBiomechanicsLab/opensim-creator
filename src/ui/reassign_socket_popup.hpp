#pragma once

#include <functional>
#include <optional>
#include <string>

namespace OpenSim {
    class Model;
    class AbstractSocket;
    class Object;
}

namespace osc::ui::reassign_socket {
    struct State final {
        std::string error;
        char search[128]{};
    };

    struct Response final {
        OpenSim::Object const& new_connectee;

        Response(OpenSim::Object const& _new_connectee) : new_connectee{_new_connectee} {
        }
    };

    // assumes caller handles ImGui::OpenPopup(modal_name);
    std::optional<Response> draw(State&, char const* modal_name, OpenSim::Model const&, OpenSim::AbstractSocket const&);
}
