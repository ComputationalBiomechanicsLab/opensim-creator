#include <oscar/Scene/SceneMesh.hpp>

#include <oscar/Maths/AABB.hpp>
#include <oscar/Maths/BVH.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Vec3.hpp>

#include <gtest/gtest.h>

#include <array>
#include <cstdint>

using osc::AABB;
using osc::BVH;
using osc::Mesh;
using osc::SceneMesh;
using osc::Vec3;

TEST(SceneMesh, GetBVHReturnsEmptyBVHOnInitialization)
{
    SceneMesh m;
    BVH const& bvh = m.getBVH();
    ASSERT_TRUE(bvh.empty());
}

TEST(SceneMesh, GetBVHReturnsExpectedRootNode)
{
    auto const pyramid = std::to_array<Vec3>(
    {
        {-1.0f, -1.0f, 0.0f},  // base: bottom-left
        { 1.0f, -1.0f, 0.0f},  // base: bottom-right
        { 0.0f,  1.0f, 0.0f},  // base: top-middle
    });
    auto const pyramidIndices = std::to_array<uint16_t>({0, 1, 2});

    Mesh m;
    m.setVerts(pyramid);
    m.setIndices(pyramidIndices);

    AABB const expectedRoot = osc::AABBFromVerts(pyramid);

    BVH bvh = SceneMesh{m}.getBVH();

    ASSERT_FALSE(bvh.empty());
    ASSERT_EQ(expectedRoot, bvh.getRootAABB());
}
