#include "OpenSimGraphicsHelpers.h"

#include <OpenSimCreator/Documents/Model/IConstModelStatePair.h>
#include <OpenSimCreator/Graphics/ComponentAbsPathDecorationTagger.h>
#include <OpenSimCreator/Graphics/ComponentSceneDecorationFlagsTagger.h>
#include <OpenSimCreator/Graphics/ModelRendererParams.h>
#include <OpenSimCreator/Graphics/OpenSimDecorationGenerator.h>

#include <oscar/Graphics/AntiAliasingLevel.h>
#include <oscar/Graphics/Scene/SceneDecoration.h>
#include <oscar/Graphics/Scene/SceneHelpers.h>
#include <oscar/Maths/Line.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/PolarPerspectiveCamera.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Utils/Perf.h>

#include <optional>
#include <vector>

using namespace osc;

SceneRendererParams osc::CalcSceneRendererParams(
    ModelRendererParams const& renderParams,
    Vec2 viewportDims,
    AntiAliasingLevel antiAliasingLevel,
    float fixupScaleFactor)
{
    SceneRendererParams params;
    if (viewportDims.x >= 1.0f && viewportDims.y >= 1.0f)
    {
        params.dimensions = viewportDims;
    }
    params.antialiasing_level = antiAliasingLevel;
    params.light_direction = recommended_light_direction(renderParams.camera);
    params.draw_floor = renderParams.renderingOptions.getDrawFloor();
    params.view_matrix = renderParams.camera.view_matrix();
    params.projection_matrix = renderParams.camera.projection_matrix(aspect_ratio(viewportDims));
    params.near_clipping_plane = renderParams.camera.znear;
    params.far_clipping_plane = renderParams.camera.zfar;
    params.view_pos = renderParams.camera.position();
    params.fixup_scale_factor = fixupScaleFactor;
    params.draw_rims = renderParams.renderingOptions.getDrawSelectionRims();
    params.draw_mesh_normals = renderParams.renderingOptions.getDrawMeshNormals();
    params.draw_shadows = renderParams.renderingOptions.getDrawShadows();
    params.light_color = renderParams.lightColor;
    params.background_color = renderParams.backgroundColor;
    params.floor_location = renderParams.floorLocation;
    return params;
}

void osc::GenerateDecorations(
    SceneCache& meshCache,
    IConstModelStatePair const& msp,
    OpenSimDecorationOptions const& options,
    std::function<void(OpenSim::Component const&, SceneDecoration&&)> const& out)
{
    ComponentAbsPathDecorationTagger pathTagger{};
    ComponentSceneDecorationFlagsTagger flagsTagger{msp.getSelected(), msp.getHovered()};

    auto callback = [pathTagger, flagsTagger, &out](OpenSim::Component const& component, SceneDecoration&& decoration) mutable
    {
        pathTagger(component, decoration);
        flagsTagger(component, decoration);
        out(component, std::move(decoration));
    };

    GenerateModelDecorations(
        meshCache,
        msp.getModel(),
        msp.getState(),
        options,
        msp.getFixupScaleFactor(),
        callback
    );
}

std::optional<SceneCollision> osc::GetClosestCollision(
    BVH const& sceneBVH,
    SceneCache& sceneCache,
    std::span<SceneDecoration const> taggedDrawlist,
    PolarPerspectiveCamera const& camera,
    Vec2 mouseScreenPos,
    Rect const& viewportScreenRect)
{
    OSC_PERF("ModelSceneDecorations/getClosestCollision");

    // un-project 2D mouse cursor into 3D scene as a ray
    Vec2 const mouseRenderPos = mouseScreenPos - viewportScreenRect.p1;
    Line const worldspaceCameraRay = camera.unproject_topleft_pos_to_world_ray(
        mouseRenderPos,
        dimensions_of(viewportScreenRect)
    );

    // find all collisions along the camera ray
    std::vector<SceneCollision> const collisions = get_all_ray_collisions_with_scene(
        sceneBVH,
        sceneCache,
        taggedDrawlist,
        worldspaceCameraRay
    );

    // filter through the collisions list
    SceneCollision const* closestCollision = nullptr;
    for (SceneCollision const& c : collisions)
    {
        if (closestCollision && c.distance_from_ray_origin > closestCollision->distance_from_ray_origin)
        {
            continue;  // it's further away than the current closest collision
        }

        SceneDecoration const& decoration = taggedDrawlist[c.decoration_index];

        if (decoration.id.empty())
        {
            continue;  // filtered out by external filter
        }

        closestCollision = &c;
    }

    return closestCollision ? *closestCollision : std::optional<SceneCollision>{};
}
