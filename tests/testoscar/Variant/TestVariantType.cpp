#include <oscar/Variant/VariantType.h>

#include <gtest/gtest.h>
#include <oscar/Utils/EnumHelpers.h>
#include <oscar/Utils/StringHelpers.h>

#include <array>
#include <limits>
#include <sstream>
#include <type_traits>

using namespace osc;

namespace
{
    struct VariantTypeStringTestCases final {
        VariantType input;
        std::string_view expectedOutput;
    };
    const auto c_expected_varianttype_strings = std::to_array<VariantTypeStringTestCases>({
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
    static_assert(num_options<VariantType>() == std::tuple_size<decltype(c_expected_varianttype_strings)>());
}

TEST(VariantType, pipe_to_ostream_works_as_intended)
{
    for (const auto& [input, expectedOutput] : c_expected_varianttype_strings) {
        std::stringstream ss;
        ss << input;
        ASSERT_EQ(ss.str(), expectedOutput);
    }
}

TEST(VariantType, ToStringReturnsExpectedResults)
{
    for (const auto& [input, expectedOutput] : c_expected_varianttype_strings) {
        ASSERT_EQ(stream_to_string(input), expectedOutput);
    }
}

TEST(VariantType, PassingBsValueIntoToStringThrows)
{
    const auto bs = static_cast<VariantType>(std::numeric_limits<std::underlying_type_t<VariantType>>::max()-1);
    ASSERT_ANY_THROW({ stream_to_string(bs); });
}
