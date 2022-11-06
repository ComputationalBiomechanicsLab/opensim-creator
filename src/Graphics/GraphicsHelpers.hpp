#pragma once

#include "src/Graphics/Material.hpp"
#include "src/Graphics/SceneCollision.hpp"
#include "src/Maths/RayCollision.hpp"

#include <glm/vec3.hpp>
#include <nonstd/span.hpp>

#include <vector>

namespace osc { struct AABB; }
namespace osc { struct BVH; }
namespace osc { struct Line; }
namespace osc { struct Transform; }
namespace osc { class Mesh; }
namespace osc { class SceneDecoration; }

namespace osc
{
    void DrawBVH(BVH const&, std::vector<SceneDecoration>&);
    void DrawAABB(AABB const&, std::vector<SceneDecoration>&);
    void DrawAABBs(nonstd::span<AABB const>, std::vector<SceneDecoration>&);
    void DrawXZFloorLines(std::vector<SceneDecoration>&, float scale = 1.0f);
    void DrawXZGrid(std::vector<SceneDecoration>&);
    void DrawXYGrid(std::vector<SceneDecoration>&);
    void DrawYZGrid(std::vector<SceneDecoration>&);

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
    osc::Material CreateWireframeOverlayMaterial();
}