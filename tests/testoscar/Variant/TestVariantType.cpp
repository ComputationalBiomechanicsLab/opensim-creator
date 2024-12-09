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
        std::string_view expected_output;
    };
    constexpr auto c_expected_variant_type_strings = std::to_array<VariantTypeStringTestCases>({
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
    static_assert(num_options<VariantType>() == std::tuple_size<decltype(c_expected_variant_type_strings)>());
}

TEST(VariantType, pipe_to_ostream_works_as_intended)
{
    for (const auto& [input, expected_output] : c_expected_variant_type_strings) {
        std::stringstream ss;
        ss << input;
        ASSERT_EQ(ss.str(), expected_output);
    }
}

TEST(VariantType, stream_to_string_returns_expected_results)
{
    for (const auto& [input, expected_output] : c_expected_variant_type_strings) {
        ASSERT_EQ(stream_to_string(input), expected_output);
    }
}

TEST(VariantType, passing_bullshit_value_into_stream_to_string_throws)
{
    const auto bs = static_cast<VariantType>(std::numeric_limits<std::underlying_type_t<VariantType>>::max()-1);  // NOLINT(clang-analyzer-optin.core.EnumCastOutOfRange)
    ASSERT_ANY_THROW({ stream_to_string(bs); });
}
