#include "src/Maths/BVH.hpp"

#include <gtest/gtest.h>

TEST(BVH, BVH_GetMaxDepthReturns0ForEmptyBVH)
{
    osc::BVH bvh;

    ASSERT_EQ(BVH_GetMaxDepth(bvh), 0);
}

TEST(BVH, BVH_GetMaxDepthReturns1ForSingleRootNode)
{
    osc::BVH bvh;

    // root (leaf)
    {
        osc::BVHNode& node = bvh.nodes.emplace_back();
        node.bounds = {};
        node.nlhs = -1;
        node.firstPrimOffset = 0;
        node.nPrims = 1;
        bvh.prims.emplace_back();
    }

    ASSERT_EQ(BVH_GetMaxDepth(bvh), 1);
}

TEST(BVH, BVH_GetMaxDepthReturns2ForBasicTree)
{
    osc::BVH bvh;

    // root (internal)
    {
        osc::BVHNode& node = bvh.nodes.emplace_back();
        node.bounds = {};
        node.nlhs = 1;
        node.firstPrimOffset = -1;
        node.nPrims = 0;
    }

    // left-hand node (leaf)
    {
        osc::BVHNode& node = bvh.nodes.emplace_back();
        node.bounds = {};
        node.nlhs = -1;
        node.firstPrimOffset = 1;
        node.nPrims = 1;
        bvh.prims.emplace_back();
    }

    // right-hand node (leaf)
    {
        osc::BVHNode& node = bvh.nodes.emplace_back();
        node.bounds = {};
        node.nlhs = -1;
        node.firstPrimOffset = 2;
        node.nPrims = 1;
        bvh.prims.emplace_back();
    }

    ASSERT_EQ(BVH_GetMaxDepth(bvh), 2);
}

TEST(BVH, BVH_GetMaxDepthReturns3ForRightHandTreeWith3Depth)
{
    osc::BVH bvh;

    // root (internal)
    {
        osc::BVHNode& node = bvh.nodes.emplace_back();
        node.bounds = {};
        node.nlhs = 5;
        node.firstPrimOffset = -1;
        node.nPrims = 0;
    }

    // left-hand node (internal, level 1)
    {
        osc::BVHNode& node = bvh.nodes.emplace_back();
        node.bounds = {};
        node.nlhs = 1;
        node.firstPrimOffset = -1;
        node.nPrims = 0;
    }

    // left-left-hand node (leaf, level 2)
    {
        osc::BVHNode& node = bvh.nodes.emplace_back();
        node.bounds = {};
        node.nlhs = -1;
        node.firstPrimOffset = 0;
        node.nPrims = 1;

        bvh.prims.emplace_back();
    }

    // left-right-hand node (internal, level 2)
    {
        osc::BVHNode& node = bvh.nodes.emplace_back();
        node.bounds = {};
        node.nlhs = 1;
        node.firstPrimOffset = -1;
        node.nPrims = 0;
    }

    // left-left-left node (leaf, level 3)
    {
        osc::BVHNode& node = bvh.nodes.emplace_back();
        node.bounds = {};
        node.nlhs = -1;
        node.firstPrimOffset = 1;
        node.nPrims = 1;

        bvh.prims.emplace_back();
    }

    // left-left-right node (leaf, level 3)
    {
        osc::BVHNode& node = bvh.nodes.emplace_back();
        node.bounds = {};
        node.nlhs = -1;
        node.firstPrimOffset = 2;
        node.nPrims = 1;

        bvh.prims.emplace_back();
    }

    // right-hand node (leaf, level 1)
    {
        osc::BVHNode& node = bvh.nodes.emplace_back();
        node.bounds = {};
        node.nlhs = -1;
        node.firstPrimOffset = 3;
        node.nPrims = 1;

        bvh.prims.emplace_back();
    }

    ASSERT_EQ(BVH_GetMaxDepth(bvh), 3);
}
