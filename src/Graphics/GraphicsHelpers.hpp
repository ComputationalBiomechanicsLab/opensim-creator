#pragma once

#include "src/Maths/RayCollision.hpp"

#include <glm/vec3.hpp>
#include <nonstd/span.hpp>

#include <cstddef>
#include <vector>

namespace osc
{
    struct AABB;
    struct BVH;
    struct Line;
    struct Transform;
    class SceneDecoration;
}

namespace osc::experimental
{
    class Mesh;
}

namespace osc
{
    void DrawBVH(BVH const&, std::vector<SceneDecoration>&);
    void DrawAABB(AABB const&, std::vector<SceneDecoration>&);
    void DrawAABBs(nonstd::span<AABB const>, std::vector<SceneDecoration>&);
    void DrawXZFloorLines(std::vector<SceneDecoration>&);
    void DrawXZGrid(std::vector<SceneDecoration>&);
    void DrawXYGrid(std::vector<SceneDecoration>&);
    void DrawYZGrid(std::vector<SceneDecoration>&);

    // updates the given BVH with the given component decorations
    void UpdateSceneBVH(nonstd::span<SceneDecoration const>, BVH& bvh);

    // describes a collision between a ray and a decoration in the scene
    struct SceneCollision final {
        glm::vec3 worldspaceLocation;
        size_t decorationIndex;
        float distanceFromRayOrigin;

        SceneCollision(glm::vec3 const& worldspaceLocation_, size_t decorationIndex_, float distanceFromRayOrigin_) :
            worldspaceLocation{worldspaceLocation_},
            decorationIndex{decorationIndex_},
            distanceFromRayOrigin{distanceFromRayOrigin_}
        {
        }
    };

    // returns all collisions along a ray
    std::vector<SceneCollision> GetAllSceneCollisions(
        BVH const& sceneBVH,
        nonstd::span<SceneDecoration const> sceneDecorations,
        Line const& worldspaceRay
    );

    // returns closest ray-triangle collision in worldspace
    RayCollision GetClosestWorldspaceRayCollision(experimental::Mesh const&, Transform const&, Line const& worldspaceRay);
}