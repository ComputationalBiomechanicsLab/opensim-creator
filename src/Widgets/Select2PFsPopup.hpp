#pragma once

#include <optional>

namespace OpenSim
{
    class PhysicalFrame;
    class Model;
}

namespace osc
{
    class Select2PFsPopup final {
    public:
        OpenSim::PhysicalFrame const* first = nullptr;
        OpenSim::PhysicalFrame const* second = nullptr;

        struct Response final {
            OpenSim::PhysicalFrame const& first;
            OpenSim::PhysicalFrame const& second;

            Response(OpenSim::PhysicalFrame const& _first, OpenSim::PhysicalFrame const& _second) :
                first{_first},
                second{_second}
            {
            }
        };

        // assumes caller has handled calling ImGui::OpenPopup(modal_name)
        std::optional<Response> draw(
                char const* popupName,
                OpenSim::Model const&,
                char const* firstLabel,
                char const* secondLabel);
    };
}
