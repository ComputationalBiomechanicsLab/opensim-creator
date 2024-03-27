#pragma once

#include <oscar/Graphics/AntiAliasingLevel.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/Material.h>
#include <oscar/Graphics/Scene/SceneCollision.h>
#include <oscar/Graphics/Scene/SceneDecoration.h>
#include <oscar/Graphics/Scene/SceneRendererParams.h>
#include <oscar/Maths/AABB.h>
#include <oscar/Maths/BVH.h>
#include <oscar/Maths/FrustumPlanes.h>
#include <oscar/Maths/Line.h>
#include <oscar/Maths/RayCollision.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>

#include <functional>
#include <optional>
#include <span>

namespace osc { struct AABB; }
namespace osc { class BVH; }
namespace osc { class Camera; }
namespace osc { class Mesh; }
namespace osc { struct PolarPerspectiveCamera; }
namespace osc { struct Rect; }
namespace osc { struct LineSegment; }
namespace osc { class SceneCache; }
namespace osc { struct Transform; }

namespace osc
{
    void drawBVH(
        SceneCache&,
        const BVH&,
        const std::function<void(SceneDecoration&&)>&
    );

    void drawAABB(
        SceneCache&,
        const AABB&,
        const std::function<void(SceneDecoration&&)>&
    );

    void drawAABBs(
        SceneCache&,
        std::span<const AABB>,
        const std::function<void(SceneDecoration&&)>&
    );

    void drawBVHLeafNodes(
        SceneCache&,
        const BVH&,
        const std::function<void(SceneDecoration&&)>&
    );

    void drawXZFloorLines(
        SceneCache&,
        const std::function<void(SceneDecoration&&)>&,
        float scale = 1.0f
    );

    void drawXZGrid(
        SceneCache&,
        const std::function<void(SceneDecoration&&)>&
    );

    void drawXYGrid(
        SceneCache&,
        const std::function<void(SceneDecoration&&)>&
    );

    void drawYZGrid(
        SceneCache&,
        const std::function<void(SceneDecoration&&)>&
    );

    struct ArrowProperties final {
        Vec3 worldspaceStart{};
        Vec3 worldspaceEnd{};
        float tipLength{};
        float neckThickness{};
        float headThickness{};
        Color color = Color::black();
    };
    void drawArrow(
        SceneCache&,
        const ArrowProperties&,
        const std::function<void(SceneDecoration&&)>&
    );

    void drawLineSegment(
        SceneCache&,
        const LineSegment&,
        const Color&,
        float radius,
        const std::function<void(SceneDecoration&&)>&
    );

    AABB getWorldspaceAABB(const SceneDecoration&);

    // updates the given BVH with the given component decorations
    void updateSceneBVH(
        std::span<const SceneDecoration>,
        BVH&
    );

    // returns all collisions along a ray
    std::vector<SceneCollision> getAllSceneCollisions(
        const BVH& sceneBVH,
        SceneCache&,
        std::span<const SceneDecoration>,
        const Line& worldspaceRay
    );

    // returns closest ray-triangle collision in worldspace
    std::optional<RayCollision> getClosestWorldspaceRayCollision(
        const Mesh&,
        const BVH&,
        const Transform&,
        const Line& worldspaceRay
    );

    // returns closest ray-triangle collision in worldspace for a given mouse position
    // within the given render rectangle
    std::optional<RayCollision> getClosestWorldspaceRayCollision(
        const PolarPerspectiveCamera&,
        const Mesh&,
        const BVH&,
        const Rect& renderScreenRect,
        Vec2 mouseScreenPos
    );

    // returns scene rendering parameters for an generic panel
    SceneRendererParams calcStandardDarkSceneRenderParams(
        const PolarPerspectiveCamera&,
        AntiAliasingLevel,
        Vec2 renderDims
    );

    // returns a triangle BVH for the given triangle mesh, or an empty BVH if the mesh is non-triangular or empty
    BVH createTriangleBVHFromMesh(const Mesh&);

    // returns a `Frustum` that represents the clipping planes of `camera` when rendering to an output that has
    // an aspect ratio of `aspectRatio`
    FrustumPlanes calcCameraFrustumPlanes(const Camera& camera, float aspectRatio);
}
