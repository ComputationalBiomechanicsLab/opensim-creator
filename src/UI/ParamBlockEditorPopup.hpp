#pragma once

namespace osc
{
    class ParamBlock;
}

namespace osc
{
    class ParamBlockEditorPopup final {
    public:
        // - assumes caller handles ImGui::OpenPopup(char const*)
        // - edits block in-place
        // - returns true if an edit was made
        bool draw(char const* popupName, ParamBlock&);
    };
}
