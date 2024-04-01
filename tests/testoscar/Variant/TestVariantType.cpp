#include <oscar/Variant/VariantType.h>

#include <gtest/gtest.h>
#include <oscar/Utils/EnumHelpers.h>

#include <array>
#include <limits>
#include <type_traits>

using namespace osc;

TEST(VariantType, ToStringReturnsExpectedResults)
{
    struct TestCase final {
        VariantType input;
        std::string_view expectedOutput;
    };
    auto const testCases = std::to_array<TestCase>(
    {
        {VariantType::Nil, "Nil"},
        {VariantType::Bool, "Bool"},
        {VariantType::Color, "Color"},
        {VariantType::Float, "Float"},
        {VariantType::Int, "Int"},
        {VariantType::String, "String"},
        {VariantType::StringName, "StringName"},
        {VariantType::Vec2, "Vec2"},
        {VariantType::Vec3, "Vec3"},
    });
    static_assert(num_options<VariantType>() == std::tuple_size<decltype(testCases)>());

    for (auto const& tc : testCases)
    {
        ASSERT_EQ(to_string(tc.input), tc.expectedOutput);
    }
}

TEST(VariantType, PassingBsValueIntoItReturnsUnknown)
{
    auto const bs = static_cast<VariantType>(std::numeric_limits<std::underlying_type_t<VariantType>>::max()-1);
    ASSERT_EQ(to_string(bs), "Unknown");
}
