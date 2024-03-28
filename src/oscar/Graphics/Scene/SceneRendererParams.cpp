#include "SceneRendererParams.h"

osc::SceneRendererParams::SceneRendererParams() :
    dimensions{1, 1},
    antialiasing_level{AntiAliasingLevel::none()},
    draw_mesh_normals{false},
    draw_rims{true},
    draw_shadows{true},
    draw_floor{true},
    near_clipping_plane{0.1f},
    far_clipping_plane{100.0f},
    view_matrix{1.0f},
    projection_matrix{1.0f},
    view_pos{0.0f, 0.0f, 0.0f},
    light_direction{-0.34f, -0.25f, 0.05f},
    light_color{default_light_color()},
    ambient_strength{0.01f},
    diffuse_strength{0.55f},
    specular_strength{0.7f},
    specular_shininess{6.0f},
    background_color{default_background_color()},
    rim_color{0.95f, 0.35f, 0.0f, 1.0f},
    rim_thickness_in_pixels{1.0f, 1.0f},
    floor_location{default_floor_location()},
    fixup_scale_factor{1.0f}
{}
