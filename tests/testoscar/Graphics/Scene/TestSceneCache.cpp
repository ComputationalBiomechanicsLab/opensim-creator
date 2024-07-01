#include <oscar/Graphics/Scene/SceneCache.h>

#include <oscar/Maths/AABB.h>
#include <oscar/Maths/BVH.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/Vec3.h>

#include <gtest/gtest.h>

#include <array>
#include <cstdint>

using namespace osc;

TEST(SceneCache, get_bvh_on_empty_mesh_returns_empty_bvh)
{
    SceneCache c;
    Mesh m;
    const BVH& bvh = c.get_bvh(m);
    ASSERT_TRUE(bvh.empty());
}

TEST(SceneCache, get_bvh_on_nonempty_mesh_returns_expected_root_node)
{
    const auto pyramid = std::to_array<Vec3>({
        {-1.0f, -1.0f, 0.0f},  // base: bottom-left
        { 1.0f, -1.0f, 0.0f},  // base: bottom-right
        { 0.0f,  1.0f, 0.0f},  // base: top-middle
    });
    const auto pyramid_indices = std::to_array<uint16_t>({0, 1, 2});

    Mesh m;
    m.set_vertices(pyramid);
    m.set_indices(pyramid_indices);

    const AABB expected_root = bounding_aabb_of(pyramid);

    SceneCache c;

    const BVH& bvh = c.get_bvh(m);

    ASSERT_FALSE(bvh.empty());
    ASSERT_EQ(expected_root, bvh.bounds());
}
