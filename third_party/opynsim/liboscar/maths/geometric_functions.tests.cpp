#include "geometric_functions.h"

#include <gtest/gtest.h>

using namespace osc;

TEST(is_parallel, returns_expected_results)
{
    // Identical directions.
    ASSERT_TRUE(is_parallel(Vector3{0.0f, 1.0f, 0.0f}, Vector3{ 0.0f,  1.0f,  0.0f}));
    // Opposite directions.
    ASSERT_TRUE(is_parallel(Vector3{0.0f, 1.0f, 0.0f}, Vector3{ 0.0f, -1.0f,  0.0f}));
    // Parallel along the X axis.
    ASSERT_TRUE(is_parallel(Vector3{1.0f, 0.0f, 0.0f}, Vector3{-1.0f,  0.0f,  0.0f}));
    // Parallel along the Z axis.
    ASSERT_TRUE(is_parallel(Vector3{0.0f, 0.0f, 1.0f}, Vector3{ 0.0f,  0.0f, -1.0f}));
    // Arbitrary parallel directions.
    ASSERT_TRUE(is_parallel(
        normalize(Vector3{ 1.0f,  2.0f,  3.0f}),
        normalize(Vector3{-1.0f, -2.0f, -3.0f})
    ));
    // Clearly non-parallel directions.
    ASSERT_FALSE(is_parallel(Vector3{1.0f, 0.0f, 0.0f}, Vector3{0.0f, 1.0f, 0.0f}));
    ASSERT_FALSE(is_parallel(Vector3{0.0f, 1.0f, 0.0f}, Vector3{0.0f, 0.0f, 1.0f}));
    // 45 degrees apart.
    ASSERT_FALSE(is_parallel(
                  Vector3{1.0f, 0.0f, 0.0f},
        normalize(Vector3{1.0f, 1.0f, 0.0f})
    ));

    // Slightly different directions but should still be parallel within tolerance.
    ASSERT_TRUE(is_parallel(
                  Vector3{1.0f, 0.0f,    0.0f},
        normalize(Vector3{1.0f, 0.0001f, 0.0f})
    ));

    // Clearly outside the default tolerance.
    ASSERT_FALSE(is_parallel(
                  Vector3{1.0f, 0.0f, 0.0f},
        normalize(Vector3{1.0f, 0.1f, 0.0f})
    ));
}

TEST(is_codirectional, returns_expected_results)
{
    // Identical directions.
    ASSERT_TRUE( is_codirectional(Vector3{0.0f, 1.0f, 0.0f}, Vector3{0.0f,  1.0f, 0.0f}));
    // Opposite directions are not codirectional.
    ASSERT_FALSE(is_codirectional(Vector3{0.0f, 1.0f, 0.0f}, Vector3{0.0f, -1.0f, 0.0f}));
    // Codirectional along the X axis.
    ASSERT_TRUE( is_codirectional(Vector3{1.0f, 0.0f, 0.0f}, Vector3{1.0f,  0.0f, 0.0f}));
    // Codirectional along the Z axis.
    ASSERT_TRUE( is_codirectional(Vector3{0.0f, 0.0f, 1.0f}, Vector3{0.0f,  0.0f, 1.0f}));
    // Arbitrary codirectional vectors.
    ASSERT_TRUE( is_codirectional(
        normalize(Vector3{1.0f, 2.0f, 3.0f}),
        normalize(Vector3{1.0f, 2.0f, 3.0f})
    ));
    // Clearly non-codirectional directions.
    ASSERT_FALSE(is_codirectional(Vector3{1.0f, 0.0f, 0.0f}, Vector3{0.0f, 1.0f, 0.0f}));
    ASSERT_FALSE(is_codirectional(Vector3{0.0f, 1.0f, 0.0f}, Vector3{0.0f, 0.0f, 1.0f}));
    // 45 degrees apart.
    ASSERT_FALSE(is_codirectional(
                  Vector3{1.0f, 0.0f, 0.0f},
        normalize(Vector3{1.0f, 1.0f, 0.0f})
    ));
    // Slightly different directions but still within tolerance.
    ASSERT_TRUE(is_codirectional(
                  Vector3{1.0f, 0.0f,    0.0f},
        normalize(Vector3{1.0f, 0.0001f, 0.0f})
    ));

    // Clearly outside the default tolerance.
    ASSERT_FALSE(is_codirectional(
                  Vector3{1.0f, 0.0f, 0.0f},
        normalize(Vector3{1.0f, 0.1f, 0.0f})
    ));
}
