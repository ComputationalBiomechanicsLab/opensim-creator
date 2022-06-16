#pragma once

#include <nonstd/span.hpp>

namespace OpenSim
{
    class PhysicalFrame;
    class Model;
}

namespace osc
{
    class Select1PFPopup final {
    public:
        // - assumes caller has handled calling ImGui::OpenPopup(char const*)
        // - returns non-nullptr if user selects a PF
        OpenSim::PhysicalFrame const* draw(
            char const* popupName,
            OpenSim::Model const&,
            nonstd::span<OpenSim::PhysicalFrame const*> exclude = {});
    };
}
