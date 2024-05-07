#pragma once

#include <oscar/UI/oscimgui.h>

namespace osc::ui
{
    bool draw_gizmo_mode_selector(
        ImGuizmo::MODE&
    );
    bool draw_gizmo_op_selector(
        ImGuizmo::OPERATION&,
        bool can_translate = true,
        bool can_rotate = true,
        bool can_scale = true
    );

    bool update_gizmo_state_from_keyboard(
        ImGuizmo::OPERATION&,
        ImGuizmo::MODE&
    );

    void set_gizmo_style_to_osc_standard();
}
