#pragma once

#include <oscar/Graphics/AntiAliasingLevel.hpp>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/Material.hpp>
#include <oscar/Maths/AABB.hpp>
#include <oscar/Maths/BVH.hpp>
#include <oscar/Maths/Line.hpp>
#include <oscar/Maths/RayCollision.hpp>
#include <oscar/Maths/Vec2.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Scene/SceneCollision.hpp>
#include <oscar/Scene/SceneDecoration.hpp>
#include <oscar/Scene/SceneRendererParams.hpp>

#include <functional>
#include <optional>
#include <span>

namespace osc { struct AABB; }
namespace osc { class AppConfig; }
namespace osc { class BVH; }
namespace osc { class Mesh; }
namespace osc { struct PolarPerspectiveCamera; }
namespace osc { struct Rect; }
namespace osc { struct Segment; }
namespace osc { class SceneCache; }
namespace osc { class ShaderCache; }
namespace osc { struct Transform; }

namespace osc
{
    void DrawBVH(
        SceneCache&,
        BVH const&,
        std::function<void(SceneDecoration&&)> const&
    );

    void DrawAABB(
        SceneCache&,
        AABB const&,
        std::function<void(SceneDecoration&&)> const&
    );

    void DrawAABBs(
        SceneCache&,
        std::span<AABB const>,
        std::function<void(SceneDecoration&&)> const&
    );

    void DrawBVHLeafNodes(
        SceneCache&,
        BVH const&,
        std::function<void(SceneDecoration&&)> const&
    );

    void DrawXZFloorLines(
        SceneCache&,
        std::function<void(SceneDecoration&&)> const&,
        float scale = 1.0f
    );

    void DrawXZGrid(
        SceneCache&,
        std::function<void(SceneDecoration&&)> const&
    );

    void DrawXYGrid(
        SceneCache&,
        std::function<void(SceneDecoration&&)> const&
    );

    void DrawYZGrid(
        SceneCache&,
        std::function<void(SceneDecoration&&)> const&
    );

    struct ArrowProperties final {
        Vec3 worldspaceStart{};
        Vec3 worldspaceEnd{};
        float tipLength{};
        float neckThickness{};
        float headThickness{};
        Color color = Color::black();
    };
    void DrawArrow(
        SceneCache&,
        ArrowProperties const&,
        std::function<void(SceneDecoration&&)> const&
    );

    void DrawLineSegment(
        SceneCache&,
        Segment const&,
        Color const&,
        float radius,
        std::function<void(SceneDecoration&&)> const&
    );

    AABB GetWorldspaceAABB(SceneDecoration const&);

    // updates the given BVH with the given component decorations
    void UpdateSceneBVH(
        std::span<SceneDecoration const>,
        BVH&
    );

    // returns all collisions along a ray
    std::vector<SceneCollision> GetAllSceneCollisions(
        BVH const& sceneBVH,
        SceneCache&,
        std::span<SceneDecoration const>,
        Line const& worldspaceRay
    );

    // returns closest ray-triangle collision in worldspace
    std::optional<RayCollision> GetClosestWorldspaceRayCollision(
        Mesh const&,
        BVH const&,
        Transform const&,
        Line const& worldspaceRay
    );

    // returns closest ray-triangle collision in worldspace for a given mouse position
    // within the given render rectangle
    std::optional<RayCollision> GetClosestWorldspaceRayCollision(
        PolarPerspectiveCamera const&,
        Mesh const&,
        BVH const&,
        Rect const& renderScreenRect,
        Vec2 mouseScreenPos
    );

    // returns scene rendering parameters for an generic panel
    SceneRendererParams CalcStandardDarkSceneRenderParams(
        PolarPerspectiveCamera const&,
        AntiAliasingLevel,
        Vec2 renderDims
    );

    // returns a material that can draw a mesh's triangles in wireframe-style
    Material CreateWireframeOverlayMaterial(
        AppConfig const&,
        ShaderCache&
    );

    // returns a triangle BVH for the given triangle mesh, or an empty BVH if the mesh is non-triangular or empty
    BVH CreateTriangleBVHFromMesh(Mesh const&);
}
