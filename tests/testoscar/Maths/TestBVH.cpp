#include <oscar/Maths/BVH.hpp>

#include <gtest/gtest.h>

TEST(BVH, GetMaxDepthReturns0OnDefaultConstruction)
{
    osc::BVH bvh;

    ASSERT_EQ(bvh.getMaxDepth(), 0);
}
