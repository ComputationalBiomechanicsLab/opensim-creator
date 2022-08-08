#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <nonstd/span.hpp>

#include <cstddef>
#include <vector>

namespace osc
{
    struct AABB;
    struct BVH;
    struct Line;
    class SceneDecoration;
}

namespace osc
{
    void DrawBVH(BVH const&, glm::mat4 const& viewMtx, glm::mat4 const& projMtx);
    void DrawAABBs(nonstd::span<AABB const>, glm::mat4 const& viewMtx, glm::mat4 const& projMtx);
    void DrawXZFloorLines(glm::mat4 const& viewMtx, glm::mat4 const& projMtx);
    void DrawXZGrid(glm::mat4 const& viewMtx, glm::mat4 const& projMtx);
    void DrawXYGrid(glm::mat4 const& viewMtx, glm::mat4 const& projMtx);
    void DrawYZGrid(glm::mat4 const& viewMtx, glm::mat4 const& projMtx);

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
}