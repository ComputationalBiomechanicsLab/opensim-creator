#pragma once

#include <oscar/UI/oscimgui.h>

namespace osc
{
    bool DrawGizmoModeSelector(
        ImGuizmo::MODE&
    );
    bool DrawGizmoOpSelector(
        ImGuizmo::OPERATION&,
        bool canTranslate = true,
        bool canRotate = true,
        bool canScale = true
    );

    bool UpdateImguizmoStateFromKeyboard(
        ImGuizmo::OPERATION&,
        ImGuizmo::MODE&
    );

    void SetImguizmoStyleToOSCStandard();
}
