#include <oscar/Maths/BVH.h>

#include <gtest/gtest.h>

using namespace osc;

TEST(BVH, max_depth_returns_zero_on_default_construction)
{
    const BVH bvh;
    ASSERT_EQ(bvh.max_depth(), 0);
}
