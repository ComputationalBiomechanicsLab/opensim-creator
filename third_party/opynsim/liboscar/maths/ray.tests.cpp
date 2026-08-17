#include "ray.h"

#include <liboscar/maths/geometric_functions.h>
#include <liboscar/maths/vector.h>

#include <gtest/gtest.h>

using namespace osc;

TEST(is_colinear_and_codirectional, returns_expected_results)
{
    // Identical rays.
    ASSERT_TRUE(is_colinear_and_codirectional(
        Ray{Vector3{0.0f, 0.0f, 0.0f}, Vector3{1.0f, 0.0f, 0.0f}},
        Ray{Vector3{0.0f, 0.0f, 0.0f}, Vector3{1.0f, 0.0f, 0.0f}}
    ));

    // Same line, same direction, different origins.
    ASSERT_TRUE(is_colinear_and_codirectional(
        Ray{Vector3{0.0f, 0.0f, 0.0f}, Vector3{1.0f, 0.0f, 0.0f}},
        Ray{Vector3{5.0f, 0.0f, 0.0f}, Vector3{1.0f, 0.0f, 0.0f}}
    ));

    // Same line, origins in the opposite direction from each other,
    // but both rays still point in the same direction.
    ASSERT_TRUE(is_colinear_and_codirectional(
        Ray{Vector3{5.0f, 0.0f, 0.0f}, Vector3{1.0f, 0.0f, 0.0f}},
        Ray{Vector3{0.0f, 0.0f, 0.0f}, Vector3{1.0f, 0.0f, 0.0f}}
    ));

    // Arbitrary colinear and codirectional rays.
    ASSERT_TRUE(is_colinear_and_codirectional(
        Ray{
            Vector3{1.0f, 2.0f, 3.0f},
            normalize(Vector3{1.0f, 2.0f, 3.0f}),
        },
        Ray{
            Vector3{5.0f, 10.0f, 15.0f},
            normalize(Vector3{1.0f, 2.0f, 3.0f}),
        }
    ));

    // Different directions, despite having the same origin.
    ASSERT_FALSE(is_colinear_and_codirectional(
        Ray{Vector3{0.0f, 0.0f, 0.0f}, Vector3{1.0f, 0.0f, 0.0f}},
        Ray{Vector3{0.0f, 0.0f, 0.0f}, Vector3{0.0f, 1.0f, 0.0f}}
    ));

    // Opposite directions: parallel, but not codirectional.
    ASSERT_FALSE(is_colinear_and_codirectional(
        Ray{Vector3{0.0f, 0.0f, 0.0f}, Vector3{ 1.0f, 0.0f, 0.0f}},
        Ray{Vector3{5.0f, 0.0f, 0.0f}, Vector3{-1.0f, 0.0f, 0.0f}}
    ));

    // Origins are not on the same line, despite identical directions.
    ASSERT_FALSE(is_colinear_and_codirectional(
        Ray{Vector3{0.0f, 0.0f, 0.0f}, Vector3{1.0f, 0.0f, 0.0f}},
        Ray{Vector3{0.0f, 1.0f, 0.0f}, Vector3{1.0f, 0.0f, 0.0f}}
    ));

    // Origins are offset perpendicular to the direction.
    ASSERT_FALSE(is_colinear_and_codirectional(
        Ray{Vector3{1.0f, 2.0f, 3.0f}, Vector3{1.0f, 0.0f, 0.0f}},
        Ray{Vector3{5.0f, 3.0f, 3.0f}, Vector3{1.0f, 0.0f, 0.0f}}
    ));

    // Directions differ slightly but remain codirectional within tolerance,
    // and the origins remain on the same line.
    ASSERT_TRUE(is_colinear_and_codirectional(
        Ray{
            Vector3{0.0f, 0.0f,    0.0f},
            normalize(Vector3{1.0f, 0.0001f, 0.0f}),
        },
        Ray{
            Vector3{5.0f, 0.0005f, 0.0f},
            normalize(Vector3{1.0f, 0.0001f, 0.0f}),
        }
    ));

    // Clearly outside the default tolerance.
    ASSERT_FALSE(is_colinear_and_codirectional(
        Ray{
            Vector3{0.0f, 0.0f, 0.0f},
            normalize(Vector3{1.0f, 0.1f, 0.0f}),
        },
        Ray{
            Vector3{5.0f, 0.0f, 0.0f},
            Vector3{1.0f, 0.0f, 0.0f},
        }
    ));
}
