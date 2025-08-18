#include "QuaternionFunctions.h"

#include <liboscar/Maths/Constants.h>
#include <liboscar/Maths/Quaternion.h>

#include <gtest/gtest.h>

#include <array>

using namespace osc;

TEST(Quaternion_isnan, returns_expected_results)
{
    struct TestCase {
        Quaternion input;
        Vec<4, bool> expected_output;
    };

    constexpr float nan = quiet_nan_v<float>;

    constexpr auto test_cases = std::to_array({
        TestCase{
            .input = {nan, 0.0f, 0.0f, 0.0f},
            .expected_output = {true, false, false, false},
        }
    });

    for (const auto& [input, expected_output] : test_cases) {
        ASSERT_EQ(isnan(input), expected_output);
    }
}
