#pragma once

#include <nonstd/span.hpp>

namespace OpenSim {
    class PhysicalFrame;
    class Model;
}

namespace osc::ui::select_1_pf {

    // - assumes caller has handled calling ImGui::OpenPopup(modal_name)
    // - returns non-nullptr if user selects a PF
    OpenSim::PhysicalFrame const* draw(
        char const* modal_name, 
        OpenSim::Model const&, 
        nonstd::span<OpenSim::PhysicalFrame const*> exclude = {});
}