#include <oscar_document/VariantType.hpp>

#include <gtest/gtest.h>
#include <oscar/Utils/Cpp20Shims.hpp>
#include <oscar/Utils/EnumHelpers.hpp>

#include <limits>
#include <type_traits>

TEST(VariantType, ToStringReturnsExpectedResults)
{
    using osc::doc::VariantType;

    struct TestCase final {
        VariantType input;
        std::string_view expectedOutput;
    };
    auto const testCases = osc::to_array<TestCase>(
    {
        {VariantType::Nil, "Nil"},
        {VariantType::Bool, "Bool"},
        {VariantType::Color, "Color"},
        {VariantType::Float, "Float"},
        {VariantType::Int, "Int"},
        {VariantType::String, "String"},
        {VariantType::StringName, "StringName"},
        {VariantType::Vec3, "Vec3"},
    });
    static_assert(osc::NumOptions<osc::doc::VariantType>() == std::tuple_size<decltype(testCases)>());

    for (auto const& tc : testCases)
    {
        ASSERT_EQ(to_string(tc.input), tc.expectedOutput);
    }
}

TEST(VariantType, PassingBsValueIntoItReturnsUnknown)
{
    auto const bs = static_cast<osc::doc::VariantType>(std::numeric_limits<std::underlying_type_t<osc::doc::VariantType>>::max()-1);
    ASSERT_EQ(to_string(bs), "Unknown");
}
