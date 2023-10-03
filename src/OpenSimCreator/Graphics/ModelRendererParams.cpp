#include "ModelRendererParams.hpp"

#include <OpenSimCreator/Graphics/CustomRenderingOptions.hpp>
#include <OpenSimCreator/Graphics/OverlayDecorationOptions.hpp>
#include <OpenSimCreator/Graphics/OpenSimDecorationOptions.hpp>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Maths/PolarPerspectiveCamera.hpp>
#include <oscar/Platform/AppConfig.hpp>
#include <oscar/Scene/SceneRendererParams.hpp>

#include <string_view>

osc::ModelRendererParams::ModelRendererParams() :
    lightColor{SceneRendererParams::DefaultLightColor()},
    backgroundColor{SceneRendererParams::DefaultBackgroundColor()},
    floorLocation{SceneRendererParams::DefaultFloorLocation()},
    camera{CreateCameraWithRadius(5.0f)}
{
}

void osc::UpdModelRendererParamsFrom(
    AppConfig const&,
    std::string_view,
    ModelRendererParams&)
{
    // out.backgroundColor = Color::red();  // HACK: see if the viewer's params are actually being loaded via this function
}

void osc::SaveModelRendererParamsDifference(
    ModelRendererParams const&,
    ModelRendererParams const&,
    std::string_view,
    AppConfig&)
{
    // TODO:
    //
    // - perform a tree crawl on the first params vs the second
    // - if there is a difference, write it to `config` as an entry

    // keys:
    //
    // - panels/viewer0/decorations/muscle_decoration_style
    // - panels/viewer0/decorations/muscle_coloring_style
    // - panels/viewer0/decorations/muscle_sizing_style
    // - panels/viewer0/decorations/show_scapulothoracic_joints
    // - panels/viewer0/decorations/show_muscle_origin_effective_line_of_action
    // - panels/viewer0/decorations/show_muscle_insertion_effective_line_of_action
    // - panels/viewer0/decorations/show_muscle_origin_anatomical_line_of_action
    // - panels/viewer0/decorations/show_muscle_insertion_anatomical_line_of_action
    // - panels/viewer0/decorations/show_centers_of_mass
    // - panels/viewer0/decorations/show_point_to_point_springs
    // - panels/viewer0/decorations/show_contact_forces

    // - panels/viewer0/overlays/draw_xz_grid
    // - panels/viewer0/overlays/draw_xy_grid
    // - panels/viewer0/overlays/draw_axis_lines
    // - panels/viewer0/overlays/draw_aabbs
    // - panels/viewer0/overlays/draw_bvh

    // - panels/viewer0/graphics/draw_floor
    // - panels/viewer0/graphics/draw_mesh_normals
    // - panels/viewer0/graphics/draw_shadows
    // - panels/viewer0/graphics/draw_selection_rims

    // - panels/viewer0/light_color
    // - panels/viewer0/background_color
    // - panels/viewer0/floor_location
    //
    // (ignore camera params)
}
