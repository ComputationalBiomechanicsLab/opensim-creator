#include "OpenSimGraphicsHelpers.h"

#include <libopensimcreator/Documents/Model/IModelStatePair.h>
#include <libopensimcreator/Graphics/ComponentAbsPathDecorationTagger.h>
#include <libopensimcreator/Graphics/ComponentSceneDecorationFlagsTagger.h>
#include <libopensimcreator/Graphics/ModelRendererParams.h>
#include <libopensimcreator/Graphics/OpenSimDecorationGenerator.h>

#include <liboscar/Graphics/AntiAliasingLevel.h>
#include <liboscar/Graphics/Scene/SceneDecoration.h>
#include <liboscar/Graphics/Scene/SceneHelpers.h>
#include <liboscar/Maths/Line.h>
#include <liboscar/Maths/MathHelpers.h>
#include <liboscar/Maths/RectFunctions.h>
#include <liboscar/Maths/PolarPerspectiveCamera.h>
#include <liboscar/Maths/Vec2.h>
#include <liboscar/Utils/Perf.h>

#include <algorithm>
#include <optional>
#include <string_view>
#include <vector>

using namespace osc;
namespace rgs = std::ranges;

namespace
{
    class PriorityOrderableCollision {
    public:
        explicit PriorityOrderableCollision(const SceneCollision& collision, const SceneDecoration& decoration) :
            collision_{&collision},
            decoration_{&decoration}
        {}

        friend bool operator<(const PriorityOrderableCollision& lhs,  const PriorityOrderableCollision& rhs)
        {
            // heuristic: if one collision has an ID that is a strict substring of the other
            // thing then it's a subcomponent, which should be prioritized for hit-testing because
            // it's likely that the user wants to actually select the more specific component (e.g.
            // when mousing over a muscle, prefer the muscle points (#592).
            const std::string_view lhsv{lhs.decoration_->id};
            const std::string_view rhsv{rhs.decoration_->id};
            if (lhsv.size() != rhsv.size()) {
                const auto shortest = std::min(lhsv.size(), rhsv.size());
                if (lhsv.substr(0, shortest) == rhsv.substr(0, shortest)) {
                    return rhsv.size() > lhsv.size();  // rhs is a strict substring of lhs (higher prio)
                }
            }

            // else: rhs is "greater than" lhs if it's closer
            return rhs.collision_->world_distance_from_ray_origin < lhs.collision_->world_distance_from_ray_origin;
        }

        const SceneCollision& collision() const { return *collision_; }
    private:
        const SceneCollision* collision_;
        const SceneDecoration* decoration_;
    };
}

SceneRendererParams osc::CalcSceneRendererParams(
    const ModelRendererParams& renderParams,
    Vec2 viewportDims,
    float viewportDevicePixelRatio,
    AntiAliasingLevel antiAliasingLevel,
    float fixupScaleFactor)
{
    SceneRendererParams rv;

    if (viewportDims.x >= 1.0f && viewportDims.y >= 1.0f) {
        rv.dimensions = viewportDims;
    }
    rv.device_pixel_ratio = viewportDevicePixelRatio;
    rv.antialiasing_level = antiAliasingLevel;
    rv.light_direction = recommended_light_direction(renderParams.camera);
    renderParams.renderingOptions.applyTo(rv);
    rv.view_matrix = renderParams.camera.view_matrix();
    rv.projection_matrix = renderParams.camera.projection_matrix(aspect_ratio_of(viewportDims));
    rv.near_clipping_plane = renderParams.camera.znear;
    rv.far_clipping_plane = renderParams.camera.zfar;
    rv.view_pos = renderParams.camera.position();
    rv.fixup_scale_factor = fixupScaleFactor;
    rv.light_color = renderParams.lightColor;
    rv.background_color = renderParams.backgroundColor;
    rv.floor_location = renderParams.floorLocation;
    return rv;
}

void osc::GenerateDecorations(
    SceneCache& meshCache,
    const IModelStatePair& msp,
    const OpenSimDecorationOptions& options,
    const std::function<void(const OpenSim::Component&, SceneDecoration&&)>& out)
{
    ComponentAbsPathDecorationTagger pathTagger{};
    ComponentSceneDecorationFlagsTagger flagsTagger{msp.getSelected(), msp.getHovered()};

    auto callback = [pathTagger, flagsTagger, &out](const OpenSim::Component& component, SceneDecoration&& decoration) mutable
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
    const BVH& sceneBVH,
    SceneCache& sceneCache,
    std::span<const SceneDecoration> taggedDrawlist,
    const PolarPerspectiveCamera& camera,
    Vec2 mouseScreenPos,
    const Rect& viewportScreenRect)
{
    OSC_PERF("osc::GetClosestCollision");

    // un-project 2D mouse cursor into 3D scene as a ray
    const Vec2 mouseRenderPos = mouseScreenPos - viewportScreenRect.p1;
    const Line worldSpaceCameraRay = camera.unproject_topleft_pos_to_world_ray(
        mouseRenderPos,
        dimensions_of(viewportScreenRect)
    );

    // find all collisions along the camera ray
    const std::vector<SceneCollision> collisions = get_all_ray_collisions_with_scene(
        sceneBVH,
        sceneCache,
        taggedDrawlist,
        worldSpaceCameraRay
    );

    // Filter through the collisions list, ensuring that hittest
    // priority is handled (#592).
    std::optional<PriorityOrderableCollision> bestCollision;
    for (const SceneCollision& c : collisions) {
        const SceneDecoration& decoration = at(taggedDrawlist, c.decoration_index);
        if (decoration.id.empty()) {
            continue;  // filter out decorations with no ID
        }
        if (const PriorityOrderableCollision sortable{c, decoration}; bestCollision < sortable) {
            bestCollision = sortable;
        }
    }

    return bestCollision ?
        std::make_optional<SceneCollision>(bestCollision->collision()) :
        std::nullopt;
}
