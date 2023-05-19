#include "oscar/Maths/BVH.hpp"

#include <gtest/gtest.h>

TEST(BVH, BVH_GetMaxDepthReturns0ForEmptyBVH)
{
    osc::BVH bvh;

    ASSERT_EQ(bvh.getMaxDepth(), 0);
}
