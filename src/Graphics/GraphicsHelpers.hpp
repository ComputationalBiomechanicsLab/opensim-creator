#pragma once

#include "src/Graphics/ImageFlags.hpp"
#include "src/Graphics/Material.hpp"
#include "src/Graphics/RenderTexture.hpp"
#include "src/Graphics/SceneCollision.hpp"
#include "src/Graphics/Texture2D.hpp"
#include "src/Maths/RayCollision.hpp"

#include <glm/vec3.hpp>
#include <nonstd/span.hpp>

#include <filesystem>
#include <optional>
#include <vector>

namespace osc { struct AABB; }
namespace osc { struct BVH; }
namespace osc { struct Line; }
namespace osc { struct Transform; }
namespace osc { class Config; }
namespace osc { class Mesh; }
namespace osc { class MeshCache; }
namespace osc { class SceneDecoration; }
namespace osc { class ShaderCache; }

namespace osc
{
    void DrawBVH(MeshCache&, BVH const&, std::vector<SceneDecoration>&);
    void DrawAABB(MeshCache&, AABB const&, std::vector<SceneDecoration>&);
    void DrawAABBs(MeshCache&, nonstd::span<AABB const>, std::vector<SceneDecoration>&);
    void DrawXZFloorLines(MeshCache&, std::vector<SceneDecoration>&, float scale = 1.0f);
    void DrawXZGrid(MeshCache&, std::vector<SceneDecoration>&);
    void DrawXYGrid(MeshCache&, std::vector<SceneDecoration>&);
    void DrawYZGrid(MeshCache&, std::vector<SceneDecoration>&);

    // updates the given BVH with the given component decorations
    void UpdateSceneBVH(nonstd::span<SceneDecoration const>, BVH& bvh);

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

    // returns a material that can draw a mesh's triangles in wireframe-style
    Material CreateWireframeOverlayMaterial(Config const&, ShaderCache&);

    // returns a texture loaded from disk via osc::Image
    Texture2D LoadTexture2DFromImage(std::filesystem::path const& path, ImageFlags = ImageFlags_None);

    void EmplaceOrReformat(std::optional<RenderTexture>& t, RenderTextureDescriptor const& desc);

    AABB GetWorldspaceAABB(SceneDecoration const&);
}