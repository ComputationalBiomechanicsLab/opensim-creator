#include <oscar/Maths/BVH.hpp>

#include <gtest/gtest.h>

using osc::BVH;

TEST(BVH, GetMaxDepthReturns0OnDefaultConstruction)
{
    BVH bvh;

    ASSERT_EQ(bvh.getMaxDepth(), 0);
}
