#pragma once

#include <glm/mat4x4.hpp>
#include <nonstd/span.hpp>

namespace osc
{
    struct AABB;
    struct BVH;
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
}