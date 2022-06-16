#pragma once

#include <string>
#include <string_view>

namespace OpenSim
{
    class Model;
    class AbstractSocket;
    class Object;
}

namespace osc
{
    class ReassignSocketPopup final {
    public:

        // assumes caller handles ImGui::OpenPopup(modal_name);
        //
        // returns != nullptr with a pointer to the new connectee if viewer chooses
        // one in UI
        OpenSim::Object const* draw(
                char const* popupName,
                OpenSim::Model const&,
                OpenSim::AbstractSocket const&);

        void clear();
        void setError(std::string_view);

    private:
        std::string error;
        char search[128]{};
    };
}
