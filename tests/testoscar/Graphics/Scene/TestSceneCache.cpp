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
    SceneCache scene_cache;
    const Mesh empty_mesh;
    const BVH& bvh = scene_cache.get_bvh(empty_mesh);
    ASSERT_TRUE(bvh.empty());
}

TEST(SceneCache, get_bvh_on_nonempty_mesh_returns_expected_root_node)
{
    const auto pyramid_vertices = std::to_array<Vec3>({
        {-1.0f, -1.0f, 0.0f},  // base: bottom-left
        { 1.0f, -1.0f, 0.0f},  // base: bottom-right
        { 0.0f,  1.0f, 0.0f},  // base: top-middle
    });
    const auto pyramid_indices = std::to_array<uint16_t>({0, 1, 2});

    Mesh pyramid_mesh;
    pyramid_mesh.set_vertices(pyramid_vertices);
    pyramid_mesh.set_indices(pyramid_indices);

    const AABB expected_root = bounding_aabb_of(pyramid_vertices);

    SceneCache scene_cache;

    const BVH& bvh = scene_cache.get_bvh(pyramid_mesh);

    ASSERT_FALSE(bvh.empty());
    ASSERT_EQ(expected_root, bvh.bounds());
}
