#pragma once

#include <oscar/UI/oscimgui.h>

namespace osc
{
    bool DrawGizmoModeSelector(
        ImGuizmo::MODE&
    );
    bool DrawGizmoOpSelector(
        ImGuizmo::OPERATION&,
        bool can_translate = true,
        bool can_rotate = true,
        bool can_scale = true
    );

    bool UpdateImguizmoStateFromKeyboard(
        ImGuizmo::OPERATION&,
        ImGuizmo::MODE&
    );

    void SetImguizmoStyleToOSCStandard();
}
