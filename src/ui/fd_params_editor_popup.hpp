#pragma once

namespace osc {
    struct FdParams;
}

namespace osc::ui::fd_params_editor_popup {
    // - assumes caller handles ImGui::OpenPopup(modal_name)
    // - edits the simulation params in-place
    // - returns true if an edit was made
    bool draw(char const* modal_name, FdParams&);
}
