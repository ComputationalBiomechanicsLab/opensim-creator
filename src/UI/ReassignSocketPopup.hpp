#pragma once

#include <string>

namespace OpenSim {
    class Model;
    class AbstractSocket;
    class Object;
}

namespace osc {

    struct ReassignSocketPopup final {
        std::string error;
        char search[128]{};

        // assumes caller handles ImGui::OpenPopup(modal_name);
        //
        // returns != nullptr with a pointer to the new connectee if viewer chooses
        // one in UI
        OpenSim::Object const* draw(
                char const* popupName,
                OpenSim::Model const&,
                OpenSim::AbstractSocket const&);
    };
}
