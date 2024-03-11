#include <oscar/Maths/PlaneFunctions.h>

#include <oscar/Maths/CommonFunctions.h>
#include <gtest/gtest.h>

#include <array>

using namespace osc;

TEST(signed_distance_between, ProducesExpectedAnswersInExampleCases)
{
    struct TestCase final {
        Plane plane;
        Vec3 point;
        float expected;
    };

    auto const cases = std::to_array<TestCase>({
         // origin    // normal                // point                 // expected signed distance
        {{Vec3{},     Vec3{0.0f, 1.0f, 0.0f}}, Vec3{0.0f,  0.5f, 0.0f},  0.5f                      },
        {{Vec3{},     Vec3{0.0f, 1.0f, 0.0f}}, Vec3{0.0f, -0.5f, 0.0f}, -0.5f                      },
        {{Vec3{1.0f}, Vec3{0.0f, 1.0f, 0.0f}}, Vec3{0.0f, 0.25f, 0.0f}, -0.75f                     },
        {{Vec3{1.0f}, Vec3{1.0f, 0.0f, 0.0f}}, Vec3{0.0f, 0.25f, 0.0f}, -1.0f                      },
    });

    for (auto const& [plane, point, expected] : cases) {
        ASSERT_NEAR(signed_distance_between(plane, point), expected, epsilon_v<float>);
    }
}
