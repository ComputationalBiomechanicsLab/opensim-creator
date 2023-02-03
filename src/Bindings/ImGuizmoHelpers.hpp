#pragma once

#include <imgui.h>
#include <ImGuizmo.h>

namespace osc
{
    void DrawGizmoModeSelector(ImGuizmo::MODE&);
    void DrawGizmoOpSelector(
        ImGuizmo::OPERATION&,
        bool canTranslate = true,
        bool canRotate = true,
        bool canScale = true
    );
}