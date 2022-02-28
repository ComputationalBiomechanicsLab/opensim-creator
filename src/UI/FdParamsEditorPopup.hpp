#pragma once

namespace osc
{
    struct FdParams;
}

namespace osc
{
    class FdParamsEditorPopup final {
    public:
        // - assumes caller handles ImGui::OpenPopup(char const*)
        // - edits the simulation params in-place
        // - returns true if an edit was made
        bool draw(char const* popupName, FdParams&);
    };
}
