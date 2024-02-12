#include <oscar/Maths/BVH.h>

#include <gtest/gtest.h>

using namespace osc;

TEST(BVH, GetMaxDepthReturns0OnDefaultConstruction)
{
    BVH bvh;

    ASSERT_EQ(bvh.getMaxDepth(), 0);
}
