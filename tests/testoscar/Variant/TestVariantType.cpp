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
    const auto testCases = std::to_array<TestCase>({
        {VariantType::None, "NoneType"},
        {VariantType::Bool, "bool"},
        {VariantType::Color, "Color"},
        {VariantType::Float, "float"},
        {VariantType::Int, "int"},
        {VariantType::String, "String"},
        {VariantType::StringName, "StringName"},
        {VariantType::Vec2, "Vec2"},
        {VariantType::Vec3, "Vec3"},
    });
    static_assert(num_options<VariantType>() == std::tuple_size<decltype(testCases)>());

    for (const auto& tc : testCases) {
        ASSERT_EQ(to_string(tc.input), tc.expectedOutput);
    }
}

TEST(VariantType, PassingBsValueIntoToStringThrows)
{
    const auto bs = static_cast<VariantType>(std::numeric_limits<std::underlying_type_t<VariantType>>::max()-1);
    ASSERT_ANY_THROW({ to_string(bs); });
}
