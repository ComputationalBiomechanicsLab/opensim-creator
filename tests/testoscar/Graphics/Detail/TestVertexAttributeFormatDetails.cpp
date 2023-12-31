#include <oscar/Graphics/Detail/VertexAttributeFormatDetails.hpp>

#include <gtest/gtest.h>
#include <oscar/Graphics/VertexAttributeFormat.hpp>
#include <oscar/Utils/EnumHelpers.hpp>

#include <array>
#include <tuple>

using osc::detail::GetDetails;
using osc::detail::VertexAttributeFormatDetails;
using osc::NumOptions;
using osc::VertexAttributeFormat;

TEST(VertexAttributeFormatDetails, SizeOfReturnsExpectedResults)
{
    // double-checks that the SizeOf function is sane - think of this as
    // a sanity check (double-book accounting) on the implementation

    struct TestCase final {
        VertexAttributeFormat format;
        VertexAttributeFormatDetails expected;
    };

    auto const testCases = std::to_array<TestCase>({
        {VertexAttributeFormat::Float32x2, {2, sizeof(float)}},
        {VertexAttributeFormat::Float32x3, {3, sizeof(float)}},
        {VertexAttributeFormat::Float32x4, {4, sizeof(float)}},
        {VertexAttributeFormat::Unorm8x4,  {4, sizeof(char)}},
    });
    static_assert(std::tuple_size_v<decltype(testCases)> == NumOptions<VertexAttributeFormat>());

    for (auto const& tc : testCases)
    {
        ASSERT_EQ(GetDetails(tc.format), tc.expected);
    }
}
