#pragma once

#include "oscar/Graphics/Color.hpp"
#include "oscar/Graphics/ImageLoadingFlags.hpp"
#include "oscar/Graphics/Material.hpp"
#include "oscar/Graphics/RenderTexture.hpp"
#include "oscar/Graphics/SceneCollision.hpp"
#include "oscar/Graphics/SceneRendererParams.hpp"
#include "oscar/Graphics/Texture2D.hpp"
#include "oscar/Maths/RayCollision.hpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <nonstd/span.hpp>

#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <vector>

namespace osc { struct AABB; }
namespace osc { class BVH; }
namespace osc { struct Line; }
namespace osc { struct Rect; }
namespace osc { struct Segment; }
namespace osc { struct Transform; }
namespace osc { class Config; }
namespace osc { class Image; }
namespace osc { class Mesh; }
namespace osc { class MeshCache; }
namespace osc { class MeshIndicesView; }
namespace osc { enum class MeshTopology; }
namespace osc { struct PolarPerspectiveCamera; }
namespace osc { struct SceneDecoration; }
namespace osc { class ShaderCache; }

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

    void DrawAABBs(
        MeshCache&,
        BVH const& drawLeafNodesOfThis,
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
        Color const& color,
        float radius,
        std::function<void(SceneDecoration&&)> const&
    );

    // updates the given BVH with the given component decorations
    void UpdateSceneBVH(
        nonstd::span<SceneDecoration const>,
        BVH&
    );

    // returns all collisions along a ray
    std::vector<SceneCollision> GetAllSceneCollisions(
        BVH const& sceneBVH,
        nonstd::span<SceneDecoration const> sceneDecorations,
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

    // returns the "mass center" of a mesh
    //
    // assumes:
    //
    // - the mesh volume has a constant density
    // - the mesh is entirely enclosed
    // - all mesh normals are correct
    glm::vec3 MassCenter(Mesh const&);

    // returns the average centerpoint of all vertices in a mesh
    glm::vec3 AverageCenterpoint(Mesh const&);

    // returns tangent vectors for the given (presumed, mesh) data
    //
    // the 4th (w) component of each vector indicates the flip direction
    // of the corresponding bitangent vector (i.e. bitangent = cross(normal, tangent) * w)
    std::vector<glm::vec4> CalcTangentVectors(
        MeshTopology const&,
        nonstd::span<glm::vec3 const> verts,
        nonstd::span<glm::vec3 const> normals,
        nonstd::span<glm::vec2 const> texCoords,
        MeshIndicesView const&
    );

    // returns a material that can draw a mesh's triangles in wireframe-style
    Material CreateWireframeOverlayMaterial(
        Config const&,
        ShaderCache&
    );

    // returns a texture loaded from the provided image data
    //
    // throws if the image isn't representable as a GPU texture
    Texture2D ToTexture2D(Image const&);

    // returns a texture loaded from disk via Image
    //
    // throws if the image data isn't representable as a GPU texture (e.g. because it has
    // an incorrect number of color channels)
    Texture2D LoadTexture2DFromImage(
        std::filesystem::path const&,
        ColorSpace,
        ImageLoadingFlags = ImageLoadingFlags::None
    );

    AABB GetWorldspaceAABB(SceneDecoration const&);

    // returns scene rendering parameters for an generic panel
    SceneRendererParams CalcStandardDarkSceneRenderParams(
        PolarPerspectiveCamera const&,
        int32_t samples,
        glm::vec2 renderDims
    );
}
