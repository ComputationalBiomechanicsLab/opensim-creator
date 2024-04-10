#include <oscar/Graphics/Scene/SceneCache.h>

#include <oscar/Maths/AABB.h>
#include <oscar/Maths/BVH.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/Vec3.h>

#include <gtest/gtest.h>

#include <array>
#include <cstdint>

using namespace osc;

TEST(SceneMesh, GetBVHOnEmptyMeshReturnsEmptyBVH)
{
    SceneCache c;
    Mesh m;
    BVH const& bvh = c.get_bvh(m);
    ASSERT_TRUE(bvh.empty());
}

TEST(SceneMesh, GetBVHOnNonEmptyMeshReturnsExpectedRootNode)
{
    auto const pyramid = std::to_array<Vec3>(
    {
        {-1.0f, -1.0f, 0.0f},  // base: bottom-left
        { 1.0f, -1.0f, 0.0f},  // base: bottom-right
        { 0.0f,  1.0f, 0.0f},  // base: top-middle
    });
    auto const pyramidIndices = std::to_array<uint16_t>({0, 1, 2});

    Mesh m;
    m.set_vertices(pyramid);
    m.set_indices(pyramidIndices);

    AABB const expectedRoot = bounding_aabb_of(pyramid);

    SceneCache c;

    BVH const& bvh = c.get_bvh(m);

    ASSERT_FALSE(bvh.empty());
    ASSERT_EQ(expectedRoot, bvh.bounds());
}
