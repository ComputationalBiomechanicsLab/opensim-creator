#pragma once

#include <oscar/Graphics/AntiAliasingLevel.hpp>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/Material.hpp>
#include <oscar/Maths/AABB.hpp>
#include <oscar/Maths/Line.hpp>
#include <oscar/Maths/RayCollision.hpp>
#include <oscar/Scene/SceneCollision.hpp>
#include <oscar/Scene/SceneRendererParams.hpp>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <nonstd/span.hpp>

#include <functional>
#include <optional>

namespace osc { struct AABB; }
namespace osc { class AppConfig; }
namespace osc { class BVH; }
namespace osc { class Mesh; }
namespace osc { class MeshCache; }
namespace osc { struct PolarPerspectiveCamera; }
namespace osc { struct Rect; }
namespace osc { struct Segment; }
namespace osc { struct SceneDecoration; }
namespace osc { class ShaderCache; }
namespace osc { struct Transform; }

namespace osc
{
    void DrawBVH(
        MeshCache&,
        BVH const&,
        std::function<void(SceneDecoration&&)> const&
    );

    void DrawAABB(
        MeshCache&,
        AABB const&,
        std::function<void(SceneDecoration&&)> const&
    );

    void DrawAABBs(
        MeshCache&,
        nonstd::span<AABB const>,
        std::function<void(SceneDecoration&&)> const&
    );

    void DrawBVHLeafNodes(
        MeshCache&,
        BVH const&,
        std::function<void(SceneDecoration&&)> const&
    );

    void DrawXZFloorLines(
        MeshCache&,
        std::function<void(SceneDecoration&&)> const&,
        float scale = 1.0f
    );

    void DrawXZGrid(
        MeshCache&,
        std::function<void(SceneDecoration&&)> const&
    );

    void DrawXYGrid(
        MeshCache&,
        std::function<void(SceneDecoration&&)> const&
    );

    void DrawYZGrid(
        MeshCache&,
        std::function<void(SceneDecoration&&)> const&
    );

    struct ArrowProperties final {
        ArrowProperties();

        glm::vec3 worldspaceStart;
        glm::vec3 worldspaceEnd;
        float tipLength;
        float neckThickness;
        float headThickness;
        Color color;
    };
    void DrawArrow(
        MeshCache&,
        ArrowProperties const&,
        std::function<void(SceneDecoration&&)> const&
    );

    void DrawLineSegment(
        MeshCache&,
        Segment const&,
        Color const&,
        float radius,
        std::function<void(SceneDecoration&&)> const&
    );

    AABB GetWorldspaceAABB(SceneDecoration const&);

    // updates the given BVH with the given component decorations
    void UpdateSceneBVH(
        nonstd::span<SceneDecoration const>,
        BVH&
    );

    // returns all collisions along a ray
    std::vector<SceneCollision> GetAllSceneCollisions(
        BVH const& sceneBVH,
        nonstd::span<SceneDecoration const>,
        Line const& worldspaceRay
    );

    // returns closest ray-triangle collision in worldspace
    std::optional<RayCollision> GetClosestWorldspaceRayCollision(
        Mesh const&,
        Transform const&,
        Line const& worldspaceRay
    );

    // returns closest ray-triangle collision in worldspace for a given mouse position
    // within the given render rectangle
    std::optional<RayCollision> GetClosestWorldspaceRayCollision(
        PolarPerspectiveCamera const&,
        Mesh const&,
        Rect const& renderScreenRect,
        glm::vec2 mouseScreenPos
    );

    // returns scene rendering parameters for an generic panel
    SceneRendererParams CalcStandardDarkSceneRenderParams(
        PolarPerspectiveCamera const&,
        AntiAliasingLevel,
        glm::vec2 renderDims
    );

    // returns a material that can draw a mesh's triangles in wireframe-style
    Material CreateWireframeOverlayMaterial(
        AppConfig const&,
        ShaderCache&
    );
}
