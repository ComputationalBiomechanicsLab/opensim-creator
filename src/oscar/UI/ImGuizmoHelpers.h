#pragma once

#include <imgui.h>
#include <ImGuizmo.h>

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
