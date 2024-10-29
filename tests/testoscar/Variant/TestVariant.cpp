#include <oscar/Variant/Variant.h>

#include <gtest/gtest.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Utils/Conversion.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/StringName.h>
#include <oscar/Utils/StringHelpers.h>

#include <array>
#include <charconv>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

using namespace osc;

namespace
{
    float to_float_or_zero(std::string_view str)
    {
        // TODO: temporarily using `std::strof` here, rather than `std::from_chars` (C++17),
        // because MacOS (Catalina) and Ubuntu 20 don't support the latter (as of Oct 2023)
        // for floating-point values

        std::string s{str};
        size_t pos = 0;
        try {
            return std::stof(s, &pos);
        }
        catch (const std::exception&) {
            return 0.0f;
        }
    }

    int to_int_or_zero(std::string_view str)
    {
        int result{};
        auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), result);
        return ec == std::errc() ? result : 0;
    }
}

TEST(Variant, is_default_constructible)
{
    static_assert(std::is_default_constructible_v<Variant>);
}

TEST(Variant, can_be_explicitly_constructed_from_bool)
{
    const Variant false_variant{false};
    ASSERT_EQ(to<bool>(false_variant), false);
    const Variant true_variant{true};
    ASSERT_EQ(to<bool>(true_variant), true);

    ASSERT_EQ(true_variant.type(), VariantType::Bool);
}

TEST(Variant, can_be_implicitly_constructed_from_bool)
{
    static_assert(std::is_convertible_v<bool, Variant>);
}

TEST(Variant, can_be_explicitly_constructed_from_Color)
{
    const Variant variant{Color::red()};
    ASSERT_EQ(to<Color>(variant), Color::red());
    ASSERT_EQ(variant.type(), VariantType::Color);
}

TEST(Variant, can_be_implicitly_constructed_from_Color)
{
    static_assert(std::is_convertible_v<Color, Variant>);
}

TEST(Variant, can_be_explicitly_constructed_from_float)
{
    const Variant variant{1.0f};
    ASSERT_EQ(to<float>(variant), 1.0f);
    ASSERT_EQ(variant.type(), VariantType::Float);
}

TEST(Variant, can_be_implicitly_constructed_from_float)
{
    static_assert(std::is_convertible_v<float, Variant>);
}

TEST(Variant, can_be_explicitly_constructed_from_int)
{
    const Variant variant{5};
    ASSERT_EQ(to<int>(variant), 5);
    ASSERT_EQ(variant.type(), VariantType::Int);
}

TEST(Variant, can_be_implicitly_constructed_from_int)
{
    static_assert(std::is_convertible_v<int, Variant>);
}

TEST(Variant, can_be_explicitly_constructed_from_a_std_string)
{
    const Variant v{std::string{"stringrval"}};
    ASSERT_EQ(to<std::string>(v), "stringrval");
    ASSERT_EQ(v.type(), VariantType::String);
}

TEST(Variant, can_be_implicitly_constructed_from_a_std_string_rvalue)
{
    static_assert(std::is_convertible_v<std::string&&, Variant>);
}

TEST(Variant, can_be_explicitly_constructed_from_a_string_literal)
{
    const Variant v{"cstringliteral"};
    ASSERT_EQ(to<std::string>(v), "cstringliteral");
    ASSERT_EQ(v.type(), VariantType::String);
}

TEST(Variant, can_be_implicitly_constructed_from_a_string_literal)
{
    static_assert(std::is_convertible_v<decltype(""), Variant>);
}

TEST(Variant, can_be_explicitly_constructed_from_a_CStringView)
{
    const Variant v{CStringView{"cstringview"}};
    ASSERT_EQ(to<std::string>(v), "cstringview");
    ASSERT_EQ(v.type(), VariantType::String);
}

TEST(Variant, can_be_implicitly_constructed_from_a_CStringView)
{
    static_assert(std::is_convertible_v<CStringView, Variant>);
}

TEST(Variant, can_be_explicitly_constructed_from_a_Vec2)
{
    const Variant v{Vec2{1.0f, 2.0f}};
    ASSERT_EQ(to<Vec2>(v), Vec2(1.0f, 2.0f));
    ASSERT_EQ(v.type(), VariantType::Vec2);
}

TEST(Variant, can_be_implicitly_constructed_from_a_Vec2)
{
    static_assert(std::is_convertible_v<Vec2, Variant>);
}

TEST(Variant, can_be_explicitly_constructed_from_a_Vec3)
{
    const Variant v{Vec3{1.0f, 2.0f, 3.0f}};
    ASSERT_EQ(to<Vec3>(v), Vec3(1.0f, 2.0f, 3.0f));
    ASSERT_EQ(v.type(), VariantType::Vec3);
}

TEST(Variant, can_be_implicitly_constructed_from_a_Vec3)
{
    static_assert(std::is_convertible_v<Vec3, Variant>);
}

TEST(Variant, default_constructor_constructs_a_Nil)
{
    ASSERT_EQ(Variant{}.type(), VariantType::None);
}

TEST(Variant, nil_value_to_bool_returns_false)
{
    ASSERT_EQ(to<bool>(Variant{}), false);
}

TEST(Variant, nil_value_to_Color_returns_black)
{
    ASSERT_EQ(to<Color>(Variant{}), Color::black());
}

TEST(Variant, nil_value_to_float_returns_zero)
{
    ASSERT_EQ(to<float>(Variant{}), 0.0f);
}

TEST(Variant, nil_value_to_int_returns_zero)
{
    ASSERT_EQ(to<int>(Variant{}), 0);
}

TEST(Variant, nil_value_to_string_returns_null_string)
{
    ASSERT_EQ(to<std::string>(Variant{}), "<null>");
}

TEST(Variant, nil_value_to_StringName_returns_empty_StringName)
{
    ASSERT_EQ(to<StringName>(Variant{}), StringName{});
}

TEST(Variant, nil_value_to_Vec2_returns_zeroed_Vec2)
{
    ASSERT_EQ(to<Vec2>(Variant{}), Vec2{});
}

TEST(Variant, nil_value_to_Vec3_returns_zeroed_Vec3)
{
    ASSERT_EQ(to<Vec3>(Variant{}), Vec3{});
}

TEST(Variant, bool_value_to_bool_returns_same_bool)
{
    ASSERT_EQ(to<bool>(Variant(false)), false);
    ASSERT_EQ(to<bool>(Variant(true)), true);
}

TEST(Variant, bool_value_to_Color_returns_black_and_white)
{
    ASSERT_EQ(to<Color>(Variant(false)), Color::black());
    ASSERT_EQ(to<Color>(Variant(true)), Color::white());
}

TEST(Variant, bool_value_to_float_returns_zero_and_one)
{
    ASSERT_EQ(to<float>(Variant(false)), 0.0f);
    ASSERT_EQ(to<float>(Variant(true)), 1.0f);
}

TEST(Variant, bool_value_to_int_returns_zero_and_one)
{
    ASSERT_EQ(to<int>(Variant(false)), 0);
    ASSERT_EQ(to<int>(Variant(true)), 1);
}

TEST(Variant, bool_value_to_string_returns_stringified_bools)
{
    const Variant vfalse{false};
    ASSERT_EQ(to<std::string>(vfalse), "false");
    const Variant vtrue{true};
    ASSERT_EQ(to<std::string>(vtrue), "true");
}

TEST(Variant, bool_value_to_StringName_returns_empty_StringName)
{
    ASSERT_EQ(to<StringName>(Variant{false}), StringName{});
    ASSERT_EQ(to<StringName>(Variant{true}), StringName{});
}

TEST(Variant, bool_value_to_Vec2_returns_zeroed_or_diagonal_one_Vec2s)
{
    ASSERT_EQ(to<Vec2>(Variant(false)), Vec2{});
    ASSERT_EQ(to<Vec2>(Variant(true)), Vec2(1.0f, 1.0f));
}

TEST(Variant, bool_value_to_Vec3_returns_zeroed_or_diagonal_Vec3s)
{
    ASSERT_EQ(to<Vec3>(Variant(false)), Vec3{});
    ASSERT_EQ(to<Vec3>(Variant(true)), Vec3(1.0f, 1.0f, 1.0f));
}

TEST(Variant, Color_to_bool_returns_false_if_black_or_true_otherwise)
{
    ASSERT_EQ(to<bool>(Variant(Color::black())), false);
    ASSERT_EQ(to<bool>(Variant(Color::white())), true);
    ASSERT_EQ(to<bool>(Variant(Color::magenta())), true);
}

TEST(Variant, Color_to_Color_returns_the_Color)
{
    ASSERT_EQ(to<Color>(Variant(Color::black())), Color::black());
    ASSERT_EQ(to<Color>(Variant(Color::red())), Color::red());
    ASSERT_EQ(to<Color>(Variant(Color::yellow())), Color::yellow());
}

TEST(Variant, Color_to_float_extracts_red_component_into_the_float)
{
    // should only extract first component, to match `Vec3` behavior for conversion
    ASSERT_EQ(to<float>(Variant(Color::black())), 0.0f);
    ASSERT_EQ(to<float>(Variant(Color::white())), 1.0f);
    ASSERT_EQ(to<float>(Variant(Color::blue())), 0.0f);
}

TEST(Variant, Color_to_int_extracts_red_component_into_the_int)
{
    // should only extract first component, to match `Vec3` behavior for conversion
    ASSERT_EQ(to<int>(Variant(Color::black())), 0);
    ASSERT_EQ(to<int>(Variant(Color::white())), 1);
    ASSERT_EQ(to<int>(Variant(Color::cyan())), 0);
    ASSERT_EQ(to<int>(Variant(Color::yellow())), 1);
}

TEST(Variant, Color_to_string_returns_html_string_representation_of_the_Color)
{
    const auto colors = std::to_array({
        Color::red(),
        Color::magenta(),
    });

    for (const auto& color : colors) {
        ASSERT_EQ(to<std::string>(Variant(color)), to_html_string_rgba(color));
    }
}

TEST(Variant, Color_to_string_returns_expected_manual_values)
{
    ASSERT_EQ(to<std::string>(Variant{Color::yellow()}), "#ffff00ff");
    ASSERT_EQ(to<std::string>(Variant{Color::magenta()}), "#ff00ffff");
}

TEST(Variant, Color_to_Vec2_extracts_rg_into_the_Vec2)
{
    ASSERT_EQ(to<Vec2>(Variant(Color(1.0f, 2.0f, 3.0f))), Vec2(1.0f, 2.0f));
    ASSERT_EQ(to<Vec2>(Variant(Color::red())), Vec2(1.0f, 0.0f));
}

TEST(Variant, Color_to_Vec3_extracts_rgb_into_the_Vec3)
{
    ASSERT_EQ(to<Vec3>(Variant(Color(1.0f, 2.0f, 3.0f))), Vec3(1.0f, 2.0f, 3.0f));
    ASSERT_EQ(to<Vec3>(Variant(Color::red())), Vec3(1.0f, 0.0f, 0.0f));
}

TEST(Variant, float_to_bool_returns_false_if_zero_otherwise_true)
{
    ASSERT_EQ(to<bool>(Variant(0.0f)), false);
    ASSERT_EQ(to<bool>(Variant(-0.5f)), true);
    ASSERT_EQ(to<bool>(Variant(-1.0f)), true);
    ASSERT_EQ(to<bool>(Variant(1.0f)), true);
    ASSERT_EQ(to<bool>(Variant(0.75f)), true);
}

TEST(Variant, float_to_Color_unpacks_the_float_into_rgb_components_of_the_Color)
{
    for (float v : std::to_array<float>({0.0f, 0.5f, 0.75, 1.0f})) {
        const Color expected = {v, v, v};
        ASSERT_EQ(to<Color>(Variant(v)), expected);
    }
}

TEST(Variant, float_to_float_returns_the_original_float)
{
    ASSERT_EQ(to<float>(Variant(0.0f)), 0.0f);
    ASSERT_EQ(to<float>(Variant(0.12345f)), 0.12345f);
    ASSERT_EQ(to<float>(Variant(-0.54321f)), -0.54321f);
}

TEST(Variant, float_to_int_returns_int_casted_equivalent_of_float)
{
    for (float v : std::to_array<float>({-0.5f, -0.123f, 0.0f, 1.0f, 1337.0f})) {
        const int expected = static_cast<int>(v);
        ASSERT_EQ(to<int>(Variant(v)), expected);
    }
}

TEST(Variant, float_to_string_returns_stringified_representation_of_the_float)
{
    for (float v : std::to_array<float>({-5.35f, -2.0f, -1.0f, 0.0f, 0.123f, 18000.0f})) {
        const std::string expected = std::to_string(v);
        ASSERT_EQ(to<std::string>(Variant(v)), expected);
    }
}

TEST(Variant, float_to_StringName_returns_empty_StringName)
{
    ASSERT_EQ(to<StringName>(Variant{0.0f}), StringName{});
    ASSERT_EQ(to<StringName>(Variant{1.0f}), StringName{});
}

TEST(Variant, float_to_Vec2_unpacks_the_float_into_all_components_of_the_Vec2)
{
    for (float v : std::to_array<float>({-20000.0f, -5.328f, -1.2f, 0.0f, 0.123f, 50.0f, 18000.0f})) {
        const Vec2 expected = {v, v};
        ASSERT_EQ(to<Vec2>(Variant(v)), expected);
    }
}

TEST(Variant, float_to_Vec3_unpacks_the_float_into_all_components_of_the_Vec3)
{
    for (float v : std::to_array<float>({-20000.0f, -5.328f, -1.2f, 0.0f, 0.123f, 50.0f, 18000.0f})) {
        const Vec3 expected = {v, v, v};
        ASSERT_EQ(to<Vec3>(Variant(v)), expected);
    }
}

TEST(Variant, int_to_bool_returns_false_if_zero_otherwise_true)
{
    ASSERT_EQ(to<bool>(Variant(0)), false);
    ASSERT_EQ(to<bool>(Variant(1)), true);
    ASSERT_EQ(to<bool>(Variant(-1)), true);
    ASSERT_EQ(to<bool>(Variant(234056)), true);
    ASSERT_EQ(to<bool>(Variant(-12938)), true);
}

TEST(Variant, int_to_Color_returns_black_if_zero_otherwise_white)
{
    ASSERT_EQ(to<Color>(Variant(0)), Color::black());
    ASSERT_EQ(to<Color>(Variant(1)), Color::white());
    ASSERT_EQ(to<Color>(Variant(-1)), Color::white());
    ASSERT_EQ(to<Color>(Variant(-230244)), Color::white());
    ASSERT_EQ(to<Color>(Variant(100983)), Color::white());
}

TEST(Variant, int_to_float_returns_int_value_casted_to_a_float)
{
    for (int v : std::to_array<int>({-10000, -1000, -1, 0, 1, 17, 23000})) {
        const auto expected = static_cast<float>(v);
        ASSERT_EQ(to<float>(Variant(v)), expected);
    }
}

TEST(Variant, int_to_int_returns_the_supplied_int)
{
    for (int v : std::to_array<int>({ -123028, -2381, -32, -2, 0, 1, 1488, 5098}))
    {
        ASSERT_EQ(to<int>(Variant(v)), v);
    }
}

TEST(Variant, int_to_string_returns_stringified_int)
{
    for (int v : std::to_array<int>({ -121010, -13482, -1923, -123, -92, -7, 0, 1, 1294, 1209849})) {
        const std::string expected = std::to_string(v);
        ASSERT_EQ(to<std::string>(Variant(v)), expected);
    }
}

TEST(Variant, int_to_StringName_returns_empty_StringName)
{
    ASSERT_EQ(to<StringName>(Variant{-1}), StringName{});
    ASSERT_EQ(to<StringName>(Variant{0}), StringName{});
    ASSERT_EQ(to<StringName>(Variant{1337}), StringName{});
}

TEST(Variant, int_to_Vec2_casts_int_to_float_and_then_unpacks_it_into_all_components_of_the_Vec2)
{
    for (int v : std::to_array<int>({ -12193, -1212, -738, -12, -1, 0, 1, 18, 1294, 1209849})) {
        const auto vf = static_cast<float>(v);
        const Vec2 expected = {vf, vf};
        ASSERT_EQ(to<Vec2>(Variant(v)), expected);
    }
}

TEST(Variant, int_to_Vec3_casts_int_to_float_and_then_unpacks_it_into_all_components_of_the_Vec3)
{
    for (int v : std::to_array<int>({ -12193, -1212, -738, -12, -1, 0, 1, 18, 1294, 1209849})) {
        const auto vf = static_cast<float>(v);
        const Vec3 expected = {vf, vf, vf};
        ASSERT_EQ(to<Vec3>(Variant(v)), expected);
    }
}

TEST(Variant, string_to_bool_returns_expected_values)
{
    ASSERT_EQ(to<bool>(Variant("false")), false);
    ASSERT_EQ(to<bool>(Variant("FALSE")), false);
    ASSERT_EQ(to<bool>(Variant("False")), false);
    ASSERT_EQ(to<bool>(Variant("FaLsE")), false);
    ASSERT_EQ(to<bool>(Variant("0")), false);
    ASSERT_EQ(to<bool>(Variant("")), false);

    // all other strings are effectively `true`
    ASSERT_EQ(to<bool>(Variant("true")), true);
    ASSERT_EQ(to<bool>(Variant("non-empty string")), true);
    ASSERT_EQ(to<bool>(Variant(" ")), true);
}

TEST(Variant, string_to_Color_works_if_string_is_a_valid_HTML_color_string)
{
    ASSERT_EQ(to<Color>(Variant{"#ff0000ff"}), Color::red());
    ASSERT_EQ(to<Color>(Variant{"#00ff00ff"}), Color::green());
    ASSERT_EQ(to<Color>(Variant{"#ffffffff"}), Color::white());
    ASSERT_EQ(to<Color>(Variant{"#00000000"}), Color::clear());
    ASSERT_EQ(to<Color>(Variant{"#000000ff"}), Color::black());
    ASSERT_EQ(to<Color>(Variant{"#000000FF"}), Color::black());
    ASSERT_EQ(to<Color>(Variant{"#123456ae"}), *try_parse_html_color_string("#123456ae"));
}

TEST(Variant, string_to_Color_returns_black_if_string_is_not_valid_HTML_color_string)
{
    ASSERT_EQ(to<Color>(Variant{"not a color"}), Color::black());
}

TEST(Variant, string_to_float_tries_to_parse_string_as_a_float_or_returns_zero_on_failure)
{
    const auto inputs = std::to_array<std::string_view>({
        "-1.0",
        "20e-10",
        "",
        "1",
        "1.0",
        "2.0",
        "not a number",
        "  ",
    });

    for (const auto& input : inputs) {
        const float expected_output = to_float_or_zero(input);
        ASSERT_EQ(to<float>(Variant{input}), expected_output);
    }
}

TEST(Variant, string_to_int_tries_to_parse_string_as_signed_base10_int)
{
    const auto inputs = std::to_array<std::string_view>({
        "-1.0",
        "20e-10",
        "",
        "1",
        "1.0",
        "2.0",
        "not a number",
        "  ",
    });

    for (const auto& input : inputs) {
        const int expected_output = to_int_or_zero(input);
        ASSERT_EQ(to<int>(Variant{input}), expected_output);
    }
}

TEST(Variant, string_to_string_returns_supplied_string)
{
    const auto inputs = std::to_array<std::string_view>({
        "some\tstring",
        "-1.0",
        "20e-10",
        "",
        "1",
        "1.0",
        "2.0",
        "not a number",
        "  ",
        "a slightly longer string in case sso is in some way important"
    });

    for (const auto& input : inputs) {
        ASSERT_EQ(to<std::string>(Variant{input}), input);
    }
}

TEST(Variant, string_to_StringName_returns_supplied_string_as_StringName)
{
    const auto inputs = std::to_array<std::string_view>({
        "some\tstring",
        "-1.0",
        "20e-10",
        "",
        "1",
        "1.0",
        "2.0",
        "not a number",
        "  ",
        "a slightly longer string in case sso is in some way important"
    });

    for (const auto& input : inputs) {
        ASSERT_EQ(to<StringName>(Variant{input}), StringName{input});
    }
}

TEST(Variant, string_to_Vec2_always_returns_zeroed_Vec2)
{
    // i.e. the converter doesn't try to parse the string
    //      in any way (yet)

    const auto inputs = std::to_array<std::string_view>({
        "some\tstring",
        "-1.0",
        "20e-10",
        "",
        "not a number",
        "  ",
        "1, 2, 3",
        "(1, 2, 3)",
        "[1, 2, 3]",
        "Vec3(1, 2, 3)",
    });

    for (const auto& input : inputs) {
        ASSERT_EQ(to<Vec2>(Variant{input}), Vec2{});
    }
}

TEST(Variant, string_to_Vec3_always_returns_zeroed_Vec3)
{
    // i.e. the converter doesn't try to parse the string
    //      in any way (yet)

    const auto inputs = std::to_array<std::string_view>({
        "some\tstring",
        "-1.0",
        "20e-10",
        "",
        "not a number",
        "  ",
        "1, 2, 3",
        "(1, 2, 3)",
        "[1, 2, 3]",
        "Vec3(1, 2, 3)",
    });

    for (const auto& input : inputs) {
        ASSERT_EQ(to<Vec3>(Variant{input}), Vec3{});
    }
}

TEST(Variant, Vec2_to_bool_returns_false_for_zeroed_Vec2)
{
    ASSERT_EQ(to<bool>(Variant{Vec2{}}), false);
}

TEST(Variant, Vec2_to_bool_returns_false_if_x_is_zero_regardless_of_the_value_of_y)
{
    // why: because it's consistent with the `toInt()` and `toFloat()` behavior, and
    // one would logically expect `if (v.to<int>())` to behave the same as `if (v.to<bool>())`
    ASSERT_EQ(to<bool>(Variant{Vec2{0.0f}}), false);
    ASSERT_EQ(to<bool>(Variant{Vec2(0.0f, 1000.0f)}), false);
    ASSERT_EQ(to<bool>(Variant{Vec2(0.0f, 7.0f)}), false);
    ASSERT_EQ(to<bool>(Variant{Vec2(0.0f, 2.0f)}), false);
    ASSERT_EQ(to<bool>(Variant{Vec2(0.0f, 1.0f)}), false);
    ASSERT_EQ(to<bool>(Variant{Vec2(0.0f, -1.0f)}), false);
    static_assert(+0.0f == -0.0f);
    ASSERT_EQ(to<bool>(Variant{Vec2(-0.0f, 1000.0f)}), false);  // how fun ;)
}

TEST(Variant, Vec2_to_bool_returns_true_if_x_is_nonzero_regardless_of_the_value_of_y)
{
    ASSERT_EQ(to<bool>(Variant{Vec2{1.0f}}), true);
    ASSERT_EQ(to<bool>(Variant{Vec2(2.0f, 7.0f)}), true);
    ASSERT_EQ(to<bool>(Variant{Vec2(30.0f, 2.0f)}), true);
    ASSERT_EQ(to<bool>(Variant{Vec2(-40.0f, 1.0f)}), true);
    ASSERT_EQ(to<bool>(Variant{Vec2(std::numeric_limits<float>::quiet_NaN(), -1.0f)}), true);
}

TEST(Variant, Vec2_to_Color_extracts_xy_into_the_Colors_rg_components)
{
    const auto test_cases = std::to_array<Vec2>({
        { 0.0f,   0.0f},
        { 1.0f,   1.0f},
        {-1.0f,   7.5f},
        { 10.0f,  0.5f},
        { 0.0f,  -0.0f},
    });

    for (const auto& test_case : test_cases) {
        ASSERT_EQ(to<Color>(Variant{test_case}), Color(test_case.x, test_case.y, 0.0f));
    }
}

TEST(Variant, Vec2_to_float_extracts_x_into_the_float)
{
    const auto test_cases = std::to_array<Vec2>({
        { 0.0f,   0.0f},
        { 1.0f,   1.0f},
        {-1.0f,   7.5f},
        { 10.0f,  0.5f},
        { 0.0f,  -0.0f},
    });

    for (const auto& test_case : test_cases) {
        ASSERT_EQ(to<float>(Variant{test_case}), test_case.x);
    }
}

TEST(Variant, Vec2_to_int_casts_x_into_an_int)
{
    const auto test_cases = std::to_array<Vec2>({
        { 0.0f,   0.0f},
        { 1.0f,   1.0f},
        {-1.0f,   7.5f},
        { 10.0f,  0.5f},
        { 0.0f,  -0.0f},
     });

    for (const auto& test_case : test_cases) {
        ASSERT_EQ(to<int>(Variant{test_case}), static_cast<int>(test_case.x));
    }
}

TEST(Variant, Vec2_to_string_returns_the_same_string_as_directly_converting_the_Vec2_into_a_String)
{
    const auto test_cases = std::to_array<Vec2>({
        { 0.0f,   0.0f},
        { 1.0f,   1.0f},
        {-1.0f,   7.5f},
        { 10.0f,  0.5f},
        { 0.0f,  -0.0f},
    });

    for (const auto& test_case : test_cases) {
        ASSERT_EQ(to<std::string>(Variant{test_case}), stream_to_string(test_case));
    }
}

TEST(Variant, Vec2_to_StringName_returns_an_empty_StringName)
{
    ASSERT_EQ(to<StringName>(Variant{Vec2{}}), StringName{});
    ASSERT_EQ(to<StringName>(Variant{Vec2(0.0f, -20.0f)}), StringName{});
}

TEST(Variant, Vec2_to_Vec2_returns_original_value_unmodified)
{
    const auto test_cases = std::to_array<Vec2>({
        { 0.0f,   0.0f},
        { 1.0f,   1.0f},
        {-1.0f,   7.5f},
        { 10.0f,  0.5f},
        { 0.0f,  -0.0f},
    });

    for (const auto& test_case : test_cases) {
        ASSERT_EQ(to<Vec2>(Variant{test_case}), test_case);
    }
}

TEST(Variant, Vec3_to_bool_returns_false_for_zeroed_Vec3)
{
    ASSERT_EQ(to<bool>(Variant{Vec3{}}), false);
}

TEST(Variant, Vec3_to_bool_returns_false_if_X_is_zero_regardless_of_the_value_of_YZ)
{
    // why: because it's consistent with the `toInt()` and `toFloat()` behavior, and
    // one would logically expect `if (v.to<int>())` to behave the same as `if (v.to<bool>())`
    ASSERT_EQ(to<bool>(Variant{Vec3{0.0f}}), false);
    ASSERT_EQ(to<bool>(Variant{Vec3(0.0f, 0.0f, 1000.0f)}), false);
    ASSERT_EQ(to<bool>(Variant{Vec3(0.0f, 7.0f, -30.0f)}), false);
    ASSERT_EQ(to<bool>(Variant{Vec3(0.0f, 2.0f, 1.0f)}), false);
    ASSERT_EQ(to<bool>(Variant{Vec3(0.0f, 1.0f, 1.0f)}), false);
    ASSERT_EQ(to<bool>(Variant{Vec3(0.0f, -1.0f, 0.0f)}), false);
    static_assert(+0.0f == -0.0f);
    ASSERT_EQ(to<bool>(Variant{Vec3(-0.0f, 0.0f, 1000.0f)}), false);  // how fun ;)
}

TEST(Variant, Vec3_to_bool_returns_true_if_X_is_nonzero_regardless_of_the_value_of_YZ)
{
    ASSERT_EQ(to<bool>(Variant{Vec3{1.0f}}), true);
    ASSERT_EQ(to<bool>(Variant{Vec3(2.0f, 7.0f, -30.0f)}), true);
    ASSERT_EQ(to<bool>(Variant{Vec3(30.0f, 2.0f, 1.0f)}), true);
    ASSERT_EQ(to<bool>(Variant{Vec3(-40.0f, 1.0f, 1.0f)}), true);
    ASSERT_EQ(to<bool>(Variant{Vec3(std::numeric_limits<float>::quiet_NaN(), -1.0f, 0.0f)}), true);
}

TEST(Variant, Vec3_to_Color_extracts_XYZ_into_RGB)
{
    const auto test_cases = std::to_array<Vec3>({
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 1.0f},
        {-1.0f, 0.0f, 1.0f},
        {10.0f, 0.0f, 7.5f},
        {0.0f, -20.0f, 0.5f},
    });

    for (const auto& test_case : test_cases) {
        ASSERT_EQ(to<Color>(Variant{test_case}), Color{test_case});
    }
}

TEST(Variant, Vec3_to_float_extracts_X_into_the_float)
{
    const auto test_cases = std::to_array<Vec3>({
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 1.0f},
        {-1.0f, 0.0f, 1.0f},
        {10.0f, 0.0f, 7.5f},
        {0.0f, -20.0f, 0.5f},
    });

    for (const auto& test_case : test_cases) {
        ASSERT_EQ(to<float>(Variant{test_case}), test_case.x);
    }
}

TEST(Variant, Vec3_to_int_extracts_x_into_the_int)
{
    const auto test_cases = std::to_array<Vec3>({
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 1.0f},
        {-1.0f, 0.0f, 1.0f},
        {10.0f, 0.0f, 7.5f},
        {0.0f, -20.0f, 0.5f},
    });

    for (const auto& test_case : test_cases) {
        ASSERT_EQ(to<int>(Variant{test_case}), static_cast<int>(test_case.x));
    }
}

TEST(Variant, Vec3_to_string_returns_the_same_string_as_directly_converting_the_Vec3_to_a_string)
{
    const auto test_cases = std::to_array<Vec3>({
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 1.0f},
        {-1.0f, 0.0f, 1.0f},
        {10.0f, 0.0f, 7.5f},
        {0.0f, -20.0f, 0.5f},
    });

    for (const auto& test_case : test_cases) {
        ASSERT_EQ(to<std::string>(Variant{test_case}), stream_to_string(test_case));
    }
}

TEST(Variant, Vec3_to_StringName_returns_an_empty_StringName)
{
    ASSERT_EQ(to<StringName>(Variant{Vec3{}}), StringName{});
    ASSERT_EQ(to<StringName>(Variant{Vec3(0.0f, -20.0f, 0.5f)}), StringName{});
}

TEST(Variant, Vec3_to_Vec3_returns_original_Vec3)
{
    const auto test_cases = std::to_array<Vec3>({
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 1.0f},
        {-1.0f, 0.0f, 1.0f},
        {10.0f, 0.0f, 7.5f},
        {0.0f, -20.0f, 0.5f},
    });

    for (const auto& test_case : test_cases) {
        ASSERT_EQ(to<Vec3>(Variant{test_case}), test_case);
    }
}

TEST(Variant, always_compares_equivalent_to_a_copy_of_itself)
{
    const auto test_cases = std::to_array<Variant>({
        Variant{false},
        Variant{true},
        Variant{Color::white()},
        Variant{Color::black()},
        Variant{Color::clear()},
        Variant{Color::magenta()},
        Variant{-1.0f},
        Variant{0.0f},
        Variant{-30.0f},
        Variant{std::numeric_limits<float>::infinity()},
        Variant{-std::numeric_limits<float>::infinity()},
        Variant{std::numeric_limits<int>::min()},
        Variant{std::numeric_limits<int>::max()},
        Variant{-1},
        Variant{0},
        Variant{1},
        Variant{""},
        Variant{"false"},
        Variant{"true"},
        Variant{"0"},
        Variant{"1"},
        Variant{"a string"},
        Variant{StringName{"a string name"}},
        Variant{Vec2{}},
        Variant{Vec2{-1.0f}},
        Variant{Vec2{0.5f}},
        Variant{Vec2{-0.5f}},
        Variant{Vec3{}},
        Variant{Vec3{1.0f}},
        Variant{Vec3{-1.0f}},
        Variant{Vec3{0.5f}},
        Variant{Vec3{-0.5f}},
    });

    for (const auto& test_case : test_cases) {
        ASSERT_EQ(test_case, test_case) << "input: " << to<std::string>(test_case);
    }

    const auto exceptional_test_cases = std::to_array<Variant>({
        Variant{std::numeric_limits<float>::quiet_NaN()},
        Variant{std::numeric_limits<float>::signaling_NaN()},
    });
    for (const auto& test_case : exceptional_test_cases) {
        ASSERT_NE(test_case, test_case) << "input: " << to<std::string>(test_case);
    }
}

TEST(Variant, is_not_equal_to_Variants_of_different_type_even_if_conversion_is_possible)
{
    const auto test_cases = std::to_array<Variant>({
        Variant{false},
        Variant{true},
        Variant{Color::white()},
        Variant{Color::black()},
        Variant{Color::clear()},
        Variant{Color::magenta()},
        Variant{-1.0f},
        Variant{0.0f},
        Variant{-30.0f},
        Variant{std::numeric_limits<float>::quiet_NaN()},
        Variant{std::numeric_limits<float>::signaling_NaN()},
        Variant{std::numeric_limits<float>::infinity()},
        Variant{-std::numeric_limits<float>::infinity()},
        Variant{std::numeric_limits<int>::min()},
        Variant{std::numeric_limits<int>::max()},
        Variant{-1},
        Variant{0},
        Variant{1},
        Variant{""},
        Variant{"false"},
        Variant{"true"},
        Variant{"0"},
        Variant{"1"},
        Variant{"a string"},
        Variant{StringName{"a stringname can be compared to a string, though"}},
        Variant{Vec2{}},
        Variant{Vec2{1.0f}},
        Variant{Vec2{-1.0f}},
        Variant{Vec2{0.5f}},
        Variant{Vec2{-0.5f}},
        Variant{Vec3{}},
        Variant{Vec3{1.0f}},
        Variant{Vec3{-1.0f}},
        Variant{Vec3{0.5f}},
        Variant{Vec3{-0.5f}},
    });

    for (size_t i = 0; i < test_cases.size(); ++i) {
        for (size_t j = 0; j != i; ++j) {
            ASSERT_NE(test_cases[i], test_cases[j]);
        }
        for (size_t j = i+1; j < test_cases.size(); ++j) {
            ASSERT_NE(test_cases[i], test_cases[j]);
        }
    }
}

TEST(Variant, can_be_hashed_with_std_hash)
{
    const auto test_cases = std::to_array<Variant>({
        Variant{false},
        Variant{true},
        Variant{Color::white()},
        Variant{Color::black()},
        Variant{Color::clear()},
        Variant{Color::magenta()},
        Variant{-1.0f},
        Variant{0.0f},
        Variant{-30.0f},
        Variant{std::numeric_limits<float>::quiet_NaN()},
        Variant{std::numeric_limits<float>::signaling_NaN()},
        Variant{std::numeric_limits<float>::infinity()},
        Variant{-std::numeric_limits<float>::infinity()},
        Variant{std::numeric_limits<int>::min()},
        Variant{std::numeric_limits<int>::max()},
        Variant{-1},
        Variant{0},
        Variant{1},
        Variant{""},
        Variant{"false"},
        Variant{"true"},
        Variant{"0"},
        Variant{"1"},
        Variant{"a string"},
        Variant{StringName{"a string name"}},
        Variant{Vec2{}},
        Variant{Vec2{1.0f}},
        Variant{Vec2{-1.0f}},
        Variant{Vec2{0.5f}},
        Variant{Vec2{-0.5f}},
        Variant{Vec3{}},
        Variant{Vec3{1.0f}},
        Variant{Vec3{-1.0f}},
        Variant{Vec3{0.5f}},
        Variant{Vec3{-0.5f}},
    });

    for (const auto& test_case : test_cases) {
        ASSERT_NO_THROW({ std::hash<Variant>{}(test_case); });
    }
}

TEST(Variant, can_be_used_as_an_argument_to_stream_to_string)
{
    const auto test_cases = std::to_array<Variant>({
        Variant{false},
        Variant{true},
        Variant{Color::white()},
        Variant{Color::black()},
        Variant{Color::clear()},
        Variant{Color::magenta()},
        Variant{-1.0f},
        Variant{0.0f},
        Variant{-30.0f},
        Variant{std::numeric_limits<float>::quiet_NaN()},
        Variant{std::numeric_limits<float>::signaling_NaN()},
        Variant{std::numeric_limits<float>::infinity()},
        Variant{-std::numeric_limits<float>::infinity()},
        Variant{std::numeric_limits<int>::min()},
        Variant{std::numeric_limits<int>::max()},
        Variant{-1},
        Variant{0},
        Variant{1},
        Variant{""},
        Variant{"false"},
        Variant{"true"},
        Variant{"0"},
        Variant{"1"},
        Variant{"a string"},
        Variant{StringName{"a string name"}},
        Variant{Vec2{}},
        Variant{Vec2{1.0f}},
        Variant{Vec2{-1.0f}},
        Variant{Vec2{0.5f}},
        Variant{Vec2{-0.5f}},
        Variant{Vec3{}},
        Variant{Vec3{1.0f}},
        Variant{Vec3{-1.0f}},
        Variant{Vec3{0.5f}},
        Variant{Vec3{-0.5f}},
    });

    for (const auto& test_case : test_cases) {
        ASSERT_EQ(stream_to_string(test_case), to<std::string>(test_case));
    }
}

TEST(Variant, writing_to_an_ostream_produces_same_output_as_converting_to_a_string)
{
    const auto test_cases = std::to_array<Variant>({
        Variant{false},
        Variant{true},
        Variant{Color::white()},
        Variant{Color::black()},
        Variant{Color::clear()},
        Variant{Color::magenta()},
        Variant{-1.0f},
        Variant{0.0f},
        Variant{-30.0f},
        Variant{std::numeric_limits<float>::quiet_NaN()},
        Variant{std::numeric_limits<float>::signaling_NaN()},
        Variant{std::numeric_limits<float>::infinity()},
        Variant{-std::numeric_limits<float>::infinity()},
        Variant{std::numeric_limits<int>::min()},
        Variant{std::numeric_limits<int>::max()},
        Variant{-1},
        Variant{0},
        Variant{1},
        Variant{""},
        Variant{"false"},
        Variant{"true"},
        Variant{"0"},
        Variant{"1"},
        Variant{"a string"},
        Variant{StringName{"a string name"}},
        Variant{Vec2{}},
        Variant{Vec2{1.0f}},
        Variant{Vec2{-1.0f}},
        Variant{Vec2{0.5f}},
        Variant{Vec2{-0.5f}},
        Variant{Vec3{}},
        Variant{Vec3{1.0f}},
        Variant{Vec3{-1.0f}},
        Variant{Vec3{0.5f}},
        Variant{Vec3{-0.5f}},
    });

    for (const auto& test_case : test_cases) {
        std::stringstream ss;
        ss << test_case;
        ASSERT_EQ(ss.str(), to<std::string>(test_case));
    }
}

TEST(Variant, std_hash_of_string_values_is_equivalent_to_hashing_the_underlying_string_value)
{
    const auto strings = std::to_array<std::string_view>({
        "false",
        "true",
        "0",
        "1",
        "a string",
    });

    for (const auto& s : strings) {
        const Variant variant{s};
        const auto hash = std::hash<Variant>{}(variant);

        ASSERT_EQ(hash, std::hash<std::string>{}(std::string{s}));
        ASSERT_EQ(hash, std::hash<std::string_view>{}(s));
        ASSERT_EQ(hash, std::hash<CStringView>{}(std::string{s}));
    }
}

TEST(Variant, type_returns_StringName_when_constructed_from_a_StringName)
{
    ASSERT_EQ(Variant(StringName{"s"}).type(), VariantType::StringName);
}

TEST(Variant, compares_equivalent_to_another_StringName_variant_with_the_same_string_content)
{
    ASSERT_EQ(Variant{StringName{"string"}}, Variant{StringName{"string"}});
}

TEST(Variant, compares_inequivalent_to_a_string_with_different_content)
{
    ASSERT_NE(Variant{StringName{"a"}}, Variant{std::string{"b"}});
}

TEST(Variant, StringName_to_bool_returns_expected_boolean_values)
{
    ASSERT_EQ(to<bool>(Variant(StringName{"false"})), false);
    ASSERT_EQ(to<bool>(Variant(StringName{"FALSE"})), false);
    ASSERT_EQ(to<bool>(Variant(StringName{"False"})), false);
    ASSERT_EQ(to<bool>(Variant(StringName{"FaLsE"})), false);
    ASSERT_EQ(to<bool>(Variant(StringName{"0"})), false);
    ASSERT_EQ(to<bool>(Variant(StringName{""})), false);

    // all other strings are effectively `true`
    ASSERT_EQ(to<bool>(Variant(StringName{"true"})), true);
    ASSERT_EQ(to<bool>(Variant(StringName{"non-empty string"})), true);
    ASSERT_EQ(to<bool>(Variant(StringName{" "})), true);
}

TEST(Variant, StringName_to_Color_works_if_string_content_is_a_valid_html_color_string)
{
    ASSERT_EQ(to<Color>(Variant{StringName{"#ff0000ff"}}), Color::red());
    ASSERT_EQ(to<Color>(Variant{StringName{"#00ff00ff"}}), Color::green());
    ASSERT_EQ(to<Color>(Variant{StringName{"#ffffffff"}}), Color::white());
    ASSERT_EQ(to<Color>(Variant{StringName{"#00000000"}}), Color::clear());
    ASSERT_EQ(to<Color>(Variant{StringName{"#000000ff"}}), Color::black());
    ASSERT_EQ(to<Color>(Variant{StringName{"#000000FF"}}), Color::black());
    ASSERT_EQ(to<Color>(Variant{StringName{"#123456ae"}}), *try_parse_html_color_string("#123456ae"));
}

TEST(Variant, StringName_to_Color_returns_black_if_string_is_an_invalid_HTML_color_string)
{
    ASSERT_EQ(to<Color>(Variant{StringName{"not a color"}}), Color::black());
}

TEST(Variant, StringName_to_float_tries_to_parse_string_content_as_float_and_returns_zero_on_failure)
{
    const auto inputs = std::to_array<std::string_view>({
        "-1.0",
        "20e-10",
        "",
        "1",
        "1.0",
        "2.0",
        "not a number",
        "  ",
    });

    for (const auto& input : inputs) {
        const float expected_output = to_float_or_zero(input);
        ASSERT_EQ(to<float>(Variant{StringName{input}}), expected_output);
    }
}

TEST(Variant, StringName_to_int_tries_to_parse_the_string_content_as_a_base10_signed_integer)
{
    const auto inputs = std::to_array<std::string_view>({
        "-1.0",
        "20e-10",
        "",
        "1",
        "1.0",
        "2.0",
        "not a number",
        "  ",
    });

    for (const auto& input : inputs) {
        const int expected_output = to_int_or_zero(input);
        ASSERT_EQ(to<int>(Variant{StringName{input}}), expected_output);
    }
}

TEST(Variant, StringName_to_string_returns_StringNames_content_in_the_string)
{
    const auto inputs = std::to_array<std::string_view>({
        "some\tstring",
        "-1.0",
        "20e-10",
        "",
        "1",
        "1.0",
        "2.0",
        "not a number",
        "  ",
        "a slightly longer string in case sso is in some way important"
    });

    for (const auto& input : inputs) {
        ASSERT_EQ(to<std::string>(Variant{StringName{input}}), input);
    }
}

TEST(Variant, StringName_to_StringName_returns_supplied_StringName)
{
    const auto inputs = std::to_array<std::string_view>({
        "some\tstring",
        "-1.0",
        "20e-10",
        "",
        "1",
        "1.0",
        "2.0",
        "not a number",
        "  ",
        "a slightly longer string in case sso is in some way important"
    });

    for (const auto& input : inputs) {
        ASSERT_EQ(to<StringName>(Variant{StringName{input}}), StringName{input});
    }
}

TEST(Variant, StringName_to_Vec3_always_returns_a_zeroed_Vec3)
{
    const auto inputs = std::to_array<std::string_view>({
        "some\tstring",
        "-1.0",
        "20e-10",
        "",
        "not a number",
        "  ",
        "1, 2, 3",
        "(1, 2, 3)",
        "[1, 2, 3]",
        "Vec3(1, 2, 3)",
    });

    for (const auto& input : inputs) {
        ASSERT_EQ(to<Vec3>(Variant{StringName{input}}), Vec3{});
    }
}

TEST(Variant, std_hash_of_StringName_is_same_as_std_hash_of_string)
{
    const auto inputs = std::to_array<std::string_view>({
        "some\tstring",
        "-1.0",
        "20e-10",
        "",
        "not a number",
        "  ",
        "1, 2, 3",
        "(1, 2, 3)",
        "[1, 2, 3]",
        "Vec3(1, 2, 3)",
    });

    for (const auto& input : inputs) {
        const auto stringname_variant = Variant{StringName{input}};
        const auto string_variant = Variant{std::string{input}};

        ASSERT_EQ(std::hash<Variant>{}(stringname_variant), std::hash<Variant>{}(string_variant));
    }
}

TEST(Variant, StringName_compares_equivalent_to_string_with_same_content)
{
    const auto inputs = std::to_array<std::string_view>({
        "some\tstring",
        "-1.0",
        "20e-10",
        "",
        "not a number",
        "  ",
        "1, 2, 3",
        "(1, 2, 3)",
        "[1, 2, 3]",
        "Vec3(1, 2, 3)",
    });

    for (const auto& input : inputs) {
        const auto stringname_variant = Variant{StringName{input}};
        const auto string_variant = Variant{std::string{input}};
        ASSERT_EQ(stringname_variant, string_variant);
    }
}

TEST(Variant, string_compares_equivalent_to_StringName_with_same_content)
{
    const auto inputs = std::to_array<std::string_view>({
        "some\tstring",
        "-1.0",
        "20e-10",
        "",
        "not a number",
        "  ",
        "1, 2, 3",
        "(1, 2, 3)",
        "[1, 2, 3]",
        "Vec3(1, 2, 3)",
    });

    for (const auto& input : inputs) {
        const auto string_variant = Variant{std::string{input}};
        const auto stringname_variant = Variant{StringName{input}};
        ASSERT_EQ(string_variant, stringname_variant);  // reversed, compared to other test
    }
}
