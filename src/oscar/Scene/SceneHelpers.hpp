#pragma once

#include <oscar/Graphics/AntiAliasingLevel.hpp>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/Material.hpp>
#include <oscar/Maths/AABB.hpp>
#include <oscar/Maths/Line.hpp>
#include <oscar/Maths/RayCollision.hpp>
#include <oscar/Maths/Vec2.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Scene/SceneCollision.hpp>
#include <oscar/Scene/SceneRendererParams.hpp>

#include <functional>
#include <optional>
#include <span>

namespace osc { struct AABB; }
namespace osc { class AppConfig; }
namespace osc { class BVH; }
namespace osc { struct PolarPerspectiveCamera; }
namespace osc { struct Rect; }
namespace osc { struct Segment; }
namespace osc { struct SceneDecoration; }
namespace osc { class SceneMesh; }
namespace osc { class SceneMeshCache; }
namespace osc { class ShaderCache; }
namespace osc { struct Transform; }

namespace osc
{
    void DrawBVH(
        SceneMeshCache&,
        BVH const&,
        std::function<void(SceneDecoration&&)> const&
    );

    void DrawAABB(
        SceneMeshCache&,
        AABB const&,
        std::function<void(SceneDecoration&&)> const&
    );

    void DrawAABBs(
        SceneMeshCache&,
        std::span<AABB const>,
        std::function<void(SceneDecoration&&)> const&
    );

    void DrawBVHLeafNodes(
        SceneMeshCache&,
        BVH const&,
        std::function<void(SceneDecoration&&)> const&
    );

    void DrawXZFloorLines(
        SceneMeshCache&,
        std::function<void(SceneDecoration&&)> const&,
        float scale = 1.0f
    );

    void DrawXZGrid(
        SceneMeshCache&,
        std::function<void(SceneDecoration&&)> const&
    );

    void DrawXYGrid(
        SceneMeshCache&,
        std::function<void(SceneDecoration&&)> const&
    );

    void DrawYZGrid(
        SceneMeshCache&,
        std::function<void(SceneDecoration&&)> const&
    );

    struct ArrowProperties final {
        ArrowProperties();

        Vec3 worldspaceStart;
        Vec3 worldspaceEnd;
        float tipLength;
        float neckThickness;
        float headThickness;
        Color color;
    };
    void DrawArrow(
        SceneMeshCache&,
        ArrowProperties const&,
        std::function<void(SceneDecoration&&)> const&
    );

    void DrawLineSegment(
        SceneMeshCache&,
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
        std::span<SceneDecoration const>,
        Line const& worldspaceRay
    );

    // returns closest ray-triangle collision in worldspace
    std::optional<RayCollision> GetClosestWorldspaceRayCollision(
        SceneMesh const&,
        Transform const&,
        Line const& worldspaceRay
    );

    // returns closest ray-triangle collision in worldspace for a given mouse position
    // within the given render rectangle
    std::optional<RayCollision> GetClosestWorldspaceRayCollision(
        PolarPerspectiveCamera const&,
        SceneMesh const&,
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
}
