#include <oscar/Maths/PlaneFunctions.h>

#include <oscar/Maths.h>
#include <gtest/gtest.h>

#include <array>

using namespace osc;

TEST(signed_distance_between, produces_expected_answers_in_precalculated_cases)
{
    struct TestCase final {
        Plane plane;
        Vec3 point;
        float expected;
    };

    const auto precalculated_cases = std::to_array<TestCase>({
         // origin    // normal                // point                 // expected signed distance
        {{Vec3{},     Vec3{0.0f, 1.0f, 0.0f}}, Vec3{0.0f,  0.5f, 0.0f},  0.5f                      },
        {{Vec3{},     Vec3{0.0f, 1.0f, 0.0f}}, Vec3{0.0f, -0.5f, 0.0f}, -0.5f                      },
        {{Vec3{1.0f}, Vec3{0.0f, 1.0f, 0.0f}}, Vec3{0.0f, 0.25f, 0.0f}, -0.75f                     },
        {{Vec3{1.0f}, Vec3{1.0f, 0.0f, 0.0f}}, Vec3{0.0f, 0.25f, 0.0f}, -1.0f                      },
    });

    for (const auto& [plane, point, expected] : precalculated_cases) {
        ASSERT_NEAR(signed_distance_between(plane, point), expected, epsilon_v<float>);
    }
}

TEST(is_in_front_of, produces_expected_answers_in_precalculated_cases)
{
    struct TestCase final {
        Plane plane;
        AABB aabb;
        bool expected;
    };

    const auto precalculated_cases = std::to_array<TestCase>({
        // origin                     // normal                  // min                 // max                  // is in front of plane
        {{Vec3{},                     Vec3{ 0.0f, 1.0f, 0.0f}}, {{ 1.0f,  1.0f,  1.0f}, { 2.0f,  2.0f,  2.0f}}, true},
        {{Vec3{},                     Vec3{ 0.0f, 1.0f, 0.0f}}, {{-2.0f, -2.0f, -2.0f}, {-1.0f, -1.0f, -1.0f}}, false},
        {{Vec3{},                     Vec3{ 1.0f, 0.0f, 0.0f}}, {{-2.0f, -2.0f, -2.0f}, {-1.0f, -1.0f, -1.0f}}, false},
        {{Vec3{},                     Vec3{-1.0f, 0.0f, 0.0f}}, {{-2.0f, -2.0f, -2.0f}, {-1.0f, -1.0f, -1.0f}}, true},
        {{Vec3{-1.0f, 0.0f, 0.0f},    Vec3{-1.0f, 0.0f, 0.0f}}, {{-2.0f, -2.0f, -2.0f}, {-1.0f, -1.0f, -1.0f}}, false},  // coincident
        {{Vec3{-0.991f, 0.0f, 0.0f},  Vec3{-1.0f, 0.0f, 0.0f}}, {{-2.0f, -2.0f, -2.0f}, {-1.0f, -1.0f, -1.0f}}, true},
        {{Vec3{-1.1f, 0.0f, 0.0f},    Vec3{-1.0f, 0.0f, 0.0f}}, {{-2.0f, -2.0f, -2.0f}, {-1.0f, -1.0f, -1.0f}}, false},
        {{Vec3{-1.9f, 0.0f, 0.0f},    Vec3{-1.0f, 0.0f, 0.0f}}, {{-2.0f, -2.0f, -2.0f}, {-1.0f, -1.0f, -1.0f}}, false},
        {{Vec3{-1.99f, 0.0f, 0.0f},   Vec3{-1.0f, 0.0f, 0.0f}}, {{-2.0f, -2.0f, -2.0f}, {-1.0f, -1.0f, -1.0f}}, false},
        {{Vec3{-2.0f, 0.0f, 0.0f},    Vec3{-1.0f, 0.0f, 0.0f}}, {{-2.0f, -2.0f, -2.0f}, {-1.0f, -1.0f, -1.0f}}, false},  // coincident
        {{Vec3{-2.01f, 0.0f, 0.0f},   Vec3{-1.0f, 0.0f, 0.0f}}, {{-2.0f, -2.0f, -2.0f}, {-1.0f, -1.0f, -1.0f}}, false},
        {{Vec3{-2.01f, 0.0f, 0.0f},   Vec3{ 1.0f, 0.0f, 0.0f}}, {{-2.0f, -2.0f, -2.0f}, {-1.0f, -1.0f, -1.0f}}, true},
        {{Vec3{-2.00f, 0.0f, 0.0f},   Vec3{ 1.0f, 0.0f, 0.0f}}, {{-2.0f, -2.0f, -2.0f}, {-1.0f, -1.0f, -1.0f}}, false},  // coincident
    });

    for (const auto& [plane, aabb, expected] : precalculated_cases) {
        ASSERT_EQ(is_in_front_of(plane, aabb), expected) << "plane = " << plane << ", aabb = " << aabb << " (dimensions_of = " << dimensions_of(aabb) << ", half_widths . normal = " << dot(half_widths_of(aabb), abs(plane.normal)) << ", signed distance = " << signed_distance_between(plane, centroid_of(aabb)) << ')';
    }
}
