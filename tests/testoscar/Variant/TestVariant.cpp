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
    Variant false_variant{false};
    ASSERT_EQ(to<bool>(false_variant), false);
    Variant true_variant{true};
    ASSERT_EQ(to<bool>(true_variant), true);

    ASSERT_EQ(true_variant.type(), VariantType::Bool);
}

TEST(Variant, can_be_implicitly_constructed_from_bool)
{
    static_assert(std::is_convertible_v<bool, Variant>);
}

TEST(Variant, can_be_explicitly_constructed_from_Color)
{
    Variant variant{Color::red()};
    ASSERT_EQ(to<Color>(variant), Color::red());
    ASSERT_EQ(variant.type(), VariantType::Color);
}

TEST(Variant, can_be_implicitly_constructed_from_Color)
{
    static_assert(std::is_convertible_v<Color, Variant>);
}

TEST(Variant, can_be_explicitly_constructed_from_float)
{
    Variant variant{1.0f};
    ASSERT_EQ(to<float>(variant), 1.0f);
    ASSERT_EQ(variant.type(), VariantType::Float);
}

TEST(Variant, can_be_implicitly_constructed_from_float)
{
    static_assert(std::is_convertible_v<float, Variant>);
}

TEST(Variant, can_be_explicitly_constructed_from_int)
{
    Variant variant{5};
    ASSERT_EQ(to<int>(variant), 5);
    ASSERT_EQ(variant.type(), VariantType::Int);
}

TEST(Variant, can_be_implicitly_constructed_from_int)
{
    static_assert(std::is_convertible_v<int, Variant>);
}

TEST(Variant, CanExplicitlyConstructFromStringRValue)
{
    Variant v{std::string{"stringrval"}};
    ASSERT_EQ(to<std::string>(v), "stringrval");
    ASSERT_EQ(v.type(), VariantType::String);
}

TEST(Variant, CanImplicitlyConstructFromStringRValue)
{
    static_assert(std::is_convertible_v<std::string&&, Variant>);
}

TEST(Variant, CanExplicitlyConstructFromStringLiteral)
{
    Variant v{"cstringliteral"};
    ASSERT_EQ(to<std::string>(v), "cstringliteral");
    ASSERT_EQ(v.type(), VariantType::String);
}

TEST(Variant, CanImplicitlyConstructFromStringLiteral)
{
    static_assert(std::is_convertible_v<decltype(""), Variant>);
}

TEST(Variant, CanExplicitlyConstructFromCStringView)
{
    Variant v{CStringView{"cstringview"}};
    ASSERT_EQ(to<std::string>(v), "cstringview");
    ASSERT_EQ(v.type(), VariantType::String);
}

TEST(Variant, CanImplicitlyConstructFromCStringView)
{
    static_assert(std::is_convertible_v<CStringView, Variant>);
}

TEST(Variant, CanExplicitlyConstructFromVec2)
{
    Variant v{Vec2{1.0f, 2.0f}};
    ASSERT_EQ(to<Vec2>(v), Vec2(1.0f, 2.0f));
    ASSERT_EQ(v.type(), VariantType::Vec2);
}

TEST(Variant, CanImplicitlyConstructFromVec2)
{
    static_assert(std::is_convertible_v<Vec2, Variant>);
}

TEST(Variant, CanExplicitlyConstructFromVec3)
{
    Variant v{Vec3{1.0f, 2.0f, 3.0f}};
    ASSERT_EQ(to<Vec3>(v), Vec3(1.0f, 2.0f, 3.0f));
    ASSERT_EQ(v.type(), VariantType::Vec3);
}

TEST(Variant, CanImplicitlyConstructFromVec3)
{
    static_assert(std::is_convertible_v<Vec3, Variant>);
}

TEST(Variant, DefaultConstructedValueIsNil)
{
    ASSERT_EQ(Variant{}.type(), VariantType::None);
}

TEST(Variant, NilValueToBoolReturnsFalse)
{
    ASSERT_EQ(to<bool>(Variant{}), false);
}

TEST(Variant, NilValueToColorReturnsBlack)
{
    ASSERT_EQ(to<Color>(Variant{}), Color::black());
}

TEST(Variant, NilValueToFloatReturnsZero)
{
    ASSERT_EQ(to<float>(Variant{}), 0.0f);
}

TEST(Variant, NilValueToIntReturnsZero)
{
    ASSERT_EQ(to<int>(Variant{}), 0);
}

TEST(Variant, NilValueToStringReturnsNull)
{
    ASSERT_EQ(to<std::string>(Variant{}), "<null>");
}

TEST(Variant, NilValueToStringNameReturnsEmptyStringName)
{
    ASSERT_EQ(to<StringName>(Variant{}), StringName{});
}

TEST(Variant, NilValueToVec2ReturnsZeroedVec2)
{
    ASSERT_EQ(to<Vec2>(Variant{}), Vec2{});
}

TEST(Variant, NilValueToVec3ReturnsZeroedVec3)
{
    ASSERT_EQ(to<Vec3>(Variant{}), Vec3{});
}

TEST(Variant, BoolValueToBoolReturnsExpectedBools)
{
    ASSERT_EQ(to<bool>(Variant(false)), false);
    ASSERT_EQ(to<bool>(Variant(true)), true);
}

TEST(Variant, BoolValueToColorReturnsExpectedColors)
{
    ASSERT_EQ(to<Color>(Variant(false)), Color::black());
    ASSERT_EQ(to<Color>(Variant(true)), Color::white());
}

TEST(Variant, BoolValueToFloatReturnsExpectedFloats)
{
    ASSERT_EQ(to<float>(Variant(false)), 0.0f);
    ASSERT_EQ(to<float>(Variant(true)), 1.0f);
}

TEST(Variant, BoolValueToIntReturnsExpectedInts)
{
    ASSERT_EQ(to<int>(Variant(false)), 0);
    ASSERT_EQ(to<int>(Variant(true)), 1);
}

TEST(Variant, BoolValueToStringReturnsExpectedStrings)
{
    Variant vfalse{false};
    ASSERT_EQ(to<std::string>(vfalse), "false");
    Variant vtrue{true};
    ASSERT_EQ(to<std::string>(vtrue), "true");
}

TEST(Variant, BoolValueToStringNameReturnsEmptyStringName)
{
    ASSERT_EQ(to<StringName>(Variant{false}), StringName{});
    ASSERT_EQ(to<StringName>(Variant{true}), StringName{});
}

TEST(Variant, BoolValueToVec2ReturnsExpectedVec2s)
{
    ASSERT_EQ(to<Vec2>(Variant(false)), Vec2{});
    ASSERT_EQ(to<Vec2>(Variant(true)), Vec2(1.0f, 1.0f));
}

TEST(Variant, BoolValueToVec3ReturnsExpectedVec3s)
{
    ASSERT_EQ(to<Vec3>(Variant(false)), Vec3{});
    ASSERT_EQ(to<Vec3>(Variant(true)), Vec3(1.0f, 1.0f, 1.0f));
}

TEST(Variant, ColorToBoolReturnsExpectedValues)
{
    ASSERT_EQ(to<bool>(Variant(Color::black())), false);
    ASSERT_EQ(to<bool>(Variant(Color::white())), true);
    ASSERT_EQ(to<bool>(Variant(Color::magenta())), true);
}

TEST(Variant, ColorToColorReturnsExpectedValues)
{
    ASSERT_EQ(to<Color>(Variant(Color::black())), Color::black());
    ASSERT_EQ(to<Color>(Variant(Color::red())), Color::red());
    ASSERT_EQ(to<Color>(Variant(Color::yellow())), Color::yellow());
}

TEST(Variant, ColorToFloatReturnsExpectedValues)
{
    // should only extract first component, to match `Vec3` behavior for conversion
    ASSERT_EQ(to<float>(Variant(Color::black())), 0.0f);
    ASSERT_EQ(to<float>(Variant(Color::white())), 1.0f);
    ASSERT_EQ(to<float>(Variant(Color::blue())), 0.0f);
}

TEST(Variant, ColorToIntReturnsExpectedValues)
{
    // should only extract first component, to match `Vec3` behavior for conversion
    ASSERT_EQ(to<int>(Variant(Color::black())), 0);
    ASSERT_EQ(to<int>(Variant(Color::white())), 1);
    ASSERT_EQ(to<int>(Variant(Color::cyan())), 0);
    ASSERT_EQ(to<int>(Variant(Color::yellow())), 1);
}

TEST(Variant, ColorValueToStringReturnsSameAsToHtmlStringRGBA)
{
    const auto colors = std::to_array({
        Color::red(),
        Color::magenta(),
    });

    for (const auto& color : colors) {
        ASSERT_EQ(to<std::string>(Variant(color)), to_html_string_rgba(color));
    }
}

TEST(Variant, ColorValueToStringReturnsExpectedManualExamples)
{
    ASSERT_EQ(to<std::string>(Variant{Color::yellow()}), "#ffff00ff");
    ASSERT_EQ(to<std::string>(Variant{Color::magenta()}), "#ff00ffff");
}

TEST(Variant, ColorValueToVec2ReturnsFirst2Components)
{
    ASSERT_EQ(to<Vec2>(Variant(Color(1.0f, 2.0f, 3.0f))), Vec2(1.0f, 2.0f));
    ASSERT_EQ(to<Vec2>(Variant(Color::red())), Vec2(1.0f, 0.0f));
}

TEST(Variant, ColorValueToVec3ReturnsFirst3Components)
{
    ASSERT_EQ(to<Vec3>(Variant(Color(1.0f, 2.0f, 3.0f))), Vec3(1.0f, 2.0f, 3.0f));
    ASSERT_EQ(to<Vec3>(Variant(Color::red())), Vec3(1.0f, 0.0f, 0.0f));
}

TEST(Variant, FloatValueToBoolReturnsExpectedValues)
{
    ASSERT_EQ(to<bool>(Variant(0.0f)), false);
    ASSERT_EQ(to<bool>(Variant(-0.5f)), true);
    ASSERT_EQ(to<bool>(Variant(-1.0f)), true);
    ASSERT_EQ(to<bool>(Variant(1.0f)), true);
    ASSERT_EQ(to<bool>(Variant(0.75f)), true);
}

TEST(Variant, FloatValueToColorReturnsExpectedColor)
{
    for (float v : std::to_array<float>({0.0f, 0.5f, 0.75, 1.0f})) {
        const Color expected = {v, v, v};
        ASSERT_EQ(to<Color>(Variant(v)), expected);
    }
}

TEST(Variant, FloatValueToFloatReturnsInput)
{
    ASSERT_EQ(to<float>(Variant(0.0f)), 0.0f);
    ASSERT_EQ(to<float>(Variant(0.12345f)), 0.12345f);
    ASSERT_EQ(to<float>(Variant(-0.54321f)), -0.54321f);
}

TEST(Variant, FloatValueToIntReturnsCastedResult)
{
    for (float v : std::to_array<float>({-0.5f, -0.123f, 0.0f, 1.0f, 1337.0f})) {
        const int expected = static_cast<int>(v);
        ASSERT_EQ(to<int>(Variant(v)), expected);
    }
}

TEST(Variant, FloatValueToStringReturnsToStringedResult)
{
    for (float v : std::to_array<float>({-5.35f, -2.0f, -1.0f, 0.0f, 0.123f, 18000.0f})) {
        const std::string expected = std::to_string(v);
        ASSERT_EQ(to<std::string>(Variant(v)), expected);
    }
}

TEST(Variant, FloatValueToStringNameReturnsEmptyStringName)
{
    ASSERT_EQ(to<StringName>(Variant{0.0f}), StringName{});
    ASSERT_EQ(to<StringName>(Variant{1.0f}), StringName{});
}

TEST(Variant, FloatValueToVec2ReturnsVec2FilledWithFloat)
{
    for (float v : std::to_array<float>({-20000.0f, -5.328f, -1.2f, 0.0f, 0.123f, 50.0f, 18000.0f})) {
        const Vec2 expected = {v, v};
        ASSERT_EQ(to<Vec2>(Variant(v)), expected);
    }
}

TEST(Variant, FloatValueToVec3ReturnsVec3FilledWithFloat)
{
    for (float v : std::to_array<float>({-20000.0f, -5.328f, -1.2f, 0.0f, 0.123f, 50.0f, 18000.0f})) {
        const Vec3 expected = {v, v, v};
        ASSERT_EQ(to<Vec3>(Variant(v)), expected);
    }
}

TEST(Variant, IntValueToBoolReturnsExpectedResults)
{
    ASSERT_EQ(to<bool>(Variant(0)), false);
    ASSERT_EQ(to<bool>(Variant(1)), true);
    ASSERT_EQ(to<bool>(Variant(-1)), true);
    ASSERT_EQ(to<bool>(Variant(234056)), true);
    ASSERT_EQ(to<bool>(Variant(-12938)), true);
}

TEST(Variant, IntValueToColorReturnsBlackOrWhite)
{
    ASSERT_EQ(to<Color>(Variant(0)), Color::black());
    ASSERT_EQ(to<Color>(Variant(1)), Color::white());
    ASSERT_EQ(to<Color>(Variant(-1)), Color::white());
    ASSERT_EQ(to<Color>(Variant(-230244)), Color::white());
    ASSERT_EQ(to<Color>(Variant(100983)), Color::white());
}

TEST(Variant, IntValueToFloatReturnsIntCastedToFloat)
{
    for (int v : std::to_array<int>({-10000, -1000, -1, 0, 1, 17, 23000})) {
        const auto expected = static_cast<float>(v);
        ASSERT_EQ(to<float>(Variant(v)), expected);
    }
}

TEST(Variant, IntValueToIntReturnsTheSuppliedInt)
{
    for (int v : std::to_array<int>({ -123028, -2381, -32, -2, 0, 1, 1488, 5098}))
    {
        ASSERT_EQ(to<int>(Variant(v)), v);
    }
}

TEST(Variant, IntValueToStringReturnsStringifiedInt)
{
    for (int v : std::to_array<int>({ -121010, -13482, -1923, -123, -92, -7, 0, 1, 1294, 1209849})) {
        const std::string expected = std::to_string(v);
        ASSERT_EQ(to<std::string>(Variant(v)), expected);
    }
}

TEST(Variant, IntValueToStringNameReturnsEmptyString)
{
    ASSERT_EQ(to<StringName>(Variant{-1}), StringName{});
    ASSERT_EQ(to<StringName>(Variant{0}), StringName{});
    ASSERT_EQ(to<StringName>(Variant{1337}), StringName{});
}

TEST(Variant, IntValueToVec2CastsValueToFloatThenPlacesItInAllSlots)
{
    for (int v : std::to_array<int>({ -12193, -1212, -738, -12, -1, 0, 1, 18, 1294, 1209849})) {
        const auto vf = static_cast<float>(v);
        const Vec2 expected = {vf, vf};
        ASSERT_EQ(to<Vec2>(Variant(v)), expected);
    }
}

TEST(Variant, IntValueToVec3CastsValueToFloatThenPlacesInAllSlots)
{
    for (int v : std::to_array<int>({ -12193, -1212, -738, -12, -1, 0, 1, 18, 1294, 1209849})) {
        const auto vf = static_cast<float>(v);
        const Vec3 expected = {vf, vf, vf};
        ASSERT_EQ(to<Vec3>(Variant(v)), expected);
    }
}

TEST(Variant, StringValueToBoolReturnsExpectedBoolValues)
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

TEST(Variant, StringValueToColorWorksIfStringIsAValidHTMLColorString)
{
    ASSERT_EQ(to<Color>(Variant{"#ff0000ff"}), Color::red());
    ASSERT_EQ(to<Color>(Variant{"#00ff00ff"}), Color::green());
    ASSERT_EQ(to<Color>(Variant{"#ffffffff"}), Color::white());
    ASSERT_EQ(to<Color>(Variant{"#00000000"}), Color::clear());
    ASSERT_EQ(to<Color>(Variant{"#000000ff"}), Color::black());
    ASSERT_EQ(to<Color>(Variant{"#000000FF"}), Color::black());
    ASSERT_EQ(to<Color>(Variant{"#123456ae"}), *try_parse_html_color_string("#123456ae"));
}

TEST(Variant, StringValueColorReturnsBlackIfStringIsInvalidHTMLColorString)
{
    ASSERT_EQ(to<Color>(Variant{"not a color"}), Color::black());
}

TEST(Variant, StringValueToFloatTriesToParseStringAsFloatAndReturnsZeroOnFailure)
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
        const float expectedOutput = to_float_or_zero(input);
        ASSERT_EQ(to<float>(Variant{input}), expectedOutput);
    }
}

TEST(Variant, StringValueToIntTriesToParseStringAsBase10Int)
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
        const int expectedOutput = to_int_or_zero(input);
        ASSERT_EQ(to<int>(Variant{input}), expectedOutput);
    }
}

TEST(Variant, StringValueToStringReturnsSuppliedString)
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

TEST(Variant, StringValueToStringNameReturnsSuppliedStringAsAStringName)
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

TEST(Variant, StringValueToVec2AlwaysReturnsZeroedVec)
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
        ASSERT_EQ(to<Vec2>(Variant{input}), Vec2{});
    }
}

TEST(Variant, StringValueToVec3AlwaysReturnsZeroedVec)
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
        ASSERT_EQ(to<Vec3>(Variant{input}), Vec3{});
    }
}

TEST(Variant, Vec2ValueToBoolReturnsFalseForZeroVec)
{
    ASSERT_EQ(to<bool>(Variant{Vec2{}}), false);
}

TEST(Variant, Vec2ValueToBoolReturnsFalseIfXIsZeroRegardlessOfOtherComponents)
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

TEST(Variant, Vec2ValueToBoolReturnsTrueIfXIsNonZeroRegardlessOfOtherComponents)
{
    ASSERT_EQ(to<bool>(Variant{Vec2{1.0f}}), true);
    ASSERT_EQ(to<bool>(Variant{Vec2(2.0f, 7.0f)}), true);
    ASSERT_EQ(to<bool>(Variant{Vec2(30.0f, 2.0f)}), true);
    ASSERT_EQ(to<bool>(Variant{Vec2(-40.0f, 1.0f)}), true);
    ASSERT_EQ(to<bool>(Variant{Vec2(std::numeric_limits<float>::quiet_NaN(), -1.0f)}), true);
}

TEST(Variant, Vec2ValueToColorExtractsTheElementsIntoRGB)
{
    const auto testCases = std::to_array<Vec2>({
        { 0.0f,   0.0f},
        { 1.0f,   1.0f},
        {-1.0f,   7.5f},
        { 10.0f,  0.5f},
        { 0.0f,  -0.0f},
    });

    for (const auto& testCase : testCases) {
        ASSERT_EQ(to<Color>(Variant{testCase}), Color(testCase.x, testCase.y, 0.0f));
    }
}

TEST(Variant, Vec2ValueToFloatExtractsXToTheFloat)
{
    const auto testCases = std::to_array<Vec2>({
        { 0.0f,   0.0f},
        { 1.0f,   1.0f},
        {-1.0f,   7.5f},
        { 10.0f,  0.5f},
        { 0.0f,  -0.0f},
    });

    for (const auto& testCase : testCases) {
        ASSERT_EQ(to<float>(Variant{testCase}), testCase.x);
    }
}

TEST(Variant, Vec2ValueToIntExtractsXToTheInt)
{
    const auto testCases = std::to_array<Vec2>({
        { 0.0f,   0.0f},
        { 1.0f,   1.0f},
        {-1.0f,   7.5f},
        { 10.0f,  0.5f},
        { 0.0f,  -0.0f},
     });

    for (const auto& testCase : testCases) {
        ASSERT_EQ(to<int>(Variant{testCase}), static_cast<int>(testCase.x));
    }
}

TEST(Variant, Vec2ValueToStringReturnsSameAsDirectlyConvertingVectorToString)
{
    const auto testCases = std::to_array<Vec2>({
        { 0.0f,   0.0f},
        { 1.0f,   1.0f},
        {-1.0f,   7.5f},
        { 10.0f,  0.5f},
        { 0.0f,  -0.0f},
    });

    for (const auto& testCase : testCases) {
        ASSERT_EQ(to<std::string>(Variant{testCase}), stream_to_string(testCase));
    }
}

TEST(Variant, Vec2ValueToStringNameReturnsAnEmptyString)
{
    ASSERT_EQ(to<StringName>(Variant{Vec2{}}), StringName{});
    ASSERT_EQ(to<StringName>(Variant{Vec2(0.0f, -20.0f)}), StringName{});
}

TEST(Variant, Vec2ValueToVec3ReturnsOriginalValue)
{
    const auto testCases = std::to_array<Vec2>({
        { 0.0f,   0.0f},
        { 1.0f,   1.0f},
        {-1.0f,   7.5f},
        { 10.0f,  0.5f},
        { 0.0f,  -0.0f},
    });

    for (const auto& testCase : testCases) {
        ASSERT_EQ(to<Vec2>(Variant{testCase}), testCase);
    }
}

TEST(Variant, Vec3ValueToBoolReturnsFalseForZeroVec)
{
    ASSERT_EQ(to<bool>(Variant{Vec3{}}), false);
}

TEST(Variant, Vec3ValueToBoolReturnsFalseIfXIsZeroRegardlessOfOtherComponents)
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

TEST(Variant, Vec3ValueToBoolReturnsTrueIfXIsNonZeroRegardlessOfOtherComponents)
{
    ASSERT_EQ(to<bool>(Variant{Vec3{1.0f}}), true);
    ASSERT_EQ(to<bool>(Variant{Vec3(2.0f, 7.0f, -30.0f)}), true);
    ASSERT_EQ(to<bool>(Variant{Vec3(30.0f, 2.0f, 1.0f)}), true);
    ASSERT_EQ(to<bool>(Variant{Vec3(-40.0f, 1.0f, 1.0f)}), true);
    ASSERT_EQ(to<bool>(Variant{Vec3(std::numeric_limits<float>::quiet_NaN(), -1.0f, 0.0f)}), true);
}

TEST(Variant, Vec3ValueToColorExtractsTheElementsIntoRGB)
{
    const auto testCases = std::to_array<Vec3>({
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 1.0f},
        {-1.0f, 0.0f, 1.0f},
        {10.0f, 0.0f, 7.5f},
        {0.0f, -20.0f, 0.5f},
    });

    for (const auto& testCase : testCases) {
        ASSERT_EQ(to<Color>(Variant{testCase}), Color{testCase});
    }
}

TEST(Variant, Vec3ValueToFloatExtractsXToTheFloat)
{
    const auto testCases = std::to_array<Vec3>({
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 1.0f},
        {-1.0f, 0.0f, 1.0f},
        {10.0f, 0.0f, 7.5f},
        {0.0f, -20.0f, 0.5f},
    });

    for (const auto& testCase : testCases) {
        ASSERT_EQ(to<float>(Variant{testCase}), testCase.x);
    }
}

TEST(Variant, Vec3ValueToIntExtractsXToTheInt)
{
    const auto testCases = std::to_array<Vec3>({
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 1.0f},
        {-1.0f, 0.0f, 1.0f},
        {10.0f, 0.0f, 7.5f},
        {0.0f, -20.0f, 0.5f},
    });

    for (const auto& testCase : testCases) {
        ASSERT_EQ(to<int>(Variant{testCase}), static_cast<int>(testCase.x));
    }
}

TEST(Variant, Vec3ValueToStringReturnsSameAsDirectlyConvertingVectorToString)
{
    const auto testCases = std::to_array<Vec3>({
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 1.0f},
        {-1.0f, 0.0f, 1.0f},
        {10.0f, 0.0f, 7.5f},
        {0.0f, -20.0f, 0.5f},
    });

    for (const auto& testCase : testCases) {
        ASSERT_EQ(to<std::string>(Variant{testCase}), stream_to_string(testCase));
    }
}

TEST(Variant, Vec3ValueToStringNameReturnsAnEmptyString)
{
    ASSERT_EQ(to<StringName>(Variant{Vec3{}}), StringName{});
    ASSERT_EQ(to<StringName>(Variant{Vec3(0.0f, -20.0f, 0.5f)}), StringName{});
}

TEST(Variant, Vec3ValueToVec3ReturnsOriginalValue)
{
    const auto testCases = std::to_array<Vec3>({
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 1.0f},
        {-1.0f, 0.0f, 1.0f},
        {10.0f, 0.0f, 7.5f},
        {0.0f, -20.0f, 0.5f},
    });

    for (const auto& testCase : testCases) {
        ASSERT_EQ(to<Vec3>(Variant{testCase}), testCase);
    }
}

TEST(Variant, IsAlwaysEqualToACopyOfItself)
{
    const auto testCases = std::to_array<Variant>({
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

    for (const auto& tc : testCases) {
        ASSERT_EQ(tc, tc) << "input: " << to<std::string>(tc);
    }

    const auto exceptions = std::to_array<Variant>({
        Variant{std::numeric_limits<float>::quiet_NaN()},
        Variant{std::numeric_limits<float>::signaling_NaN()},
    });
    for (const auto& tc : exceptions) {
        ASSERT_NE(tc, tc) << "input: " << to<std::string>(tc);
    }
}

TEST(Variant, IsNotEqualToOtherValuesEvenIfConversionIsPossible)
{
    const auto testCases = std::to_array<Variant>({
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

    for (size_t i = 0; i < testCases.size(); ++i)
    {
        for (size_t j = 0; j != i; ++j)
        {
            ASSERT_NE(testCases[i], testCases[j]);
        }
        for (size_t j = i+1; j < testCases.size(); ++j)
        {
            ASSERT_NE(testCases[i], testCases[j]);
        }
    }
}

TEST(Variant, CanHashAVarietyOfTypes)
{
    const auto testCases = std::to_array<Variant>({
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

    for (const auto& testCase : testCases) {
        ASSERT_NO_THROW({ std::hash<Variant>{}(testCase); });
    }
}

TEST(Variant, FreeFunctionToStringOnVarietyOfTypesReturnsSameAsCallingToStringMemberFunction)
{
    const auto testCases = std::to_array<Variant>({
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

    for (const auto& testCase : testCases) {
        ASSERT_EQ(stream_to_string(testCase), to<std::string>(testCase));
    }
}

TEST(Variant, StreamingToOutputStreamProducesSameOutputAsToString)
{
    const auto testCases = std::to_array<Variant>({
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

    for (const auto& testCase : testCases) {
        std::stringstream ss;
        ss << testCase;
        ASSERT_EQ(ss.str(), to<std::string>(testCase));
    }
}

TEST(Variant, HashesForStringValuesMatchStdStringEtc)
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

TEST(Variant, ConstructingFromstringNameMakesGetTypeReturnStringNameType)
{
    ASSERT_EQ(Variant(StringName{"s"}).type(), VariantType::StringName);
}

TEST(Variant, ConstructedFromSameStringNameComparesEquivalent)
{
    ASSERT_EQ(Variant{StringName{"string"}}, Variant{StringName{"string"}});
}

TEST(Variant, ConstructedFromStringNameComparesInequivalentToVariantConstructedFromDifferentString)
{
    ASSERT_NE(Variant{StringName{"a"}}, Variant{std::string{"b"}});
}

TEST(Variant, StringNameValueToBoolReturnsExpectedBoolValues)
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

TEST(Variant, StringNameValueToColorWorksIfStringIsAValidHTMLColorString)
{
    ASSERT_EQ(to<Color>(Variant{StringName{"#ff0000ff"}}), Color::red());
    ASSERT_EQ(to<Color>(Variant{StringName{"#00ff00ff"}}), Color::green());
    ASSERT_EQ(to<Color>(Variant{StringName{"#ffffffff"}}), Color::white());
    ASSERT_EQ(to<Color>(Variant{StringName{"#00000000"}}), Color::clear());
    ASSERT_EQ(to<Color>(Variant{StringName{"#000000ff"}}), Color::black());
    ASSERT_EQ(to<Color>(Variant{StringName{"#000000FF"}}), Color::black());
    ASSERT_EQ(to<Color>(Variant{StringName{"#123456ae"}}), *try_parse_html_color_string("#123456ae"));
}

TEST(Variant, StringNameValueToColorReturnsBlackIfStringIsInvalidHTMLColorString)
{
    ASSERT_EQ(to<Color>(Variant{StringName{"not a color"}}), Color::black());
}

TEST(Variant, StringNameValueToFloatTriesToParseStringAsFloatAndReturnsZeroOnFailure)
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
        const float expectedOutput = to_float_or_zero(input);
        ASSERT_EQ(to<float>(Variant{StringName{input}}), expectedOutput);
    }
}

TEST(Variant, StringNameValueToIntTriesToParseStringAsBase10Int)
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
        const int expectedOutput = to_int_or_zero(input);
        ASSERT_EQ(to<int>(Variant{StringName{input}}), expectedOutput);
    }
}

TEST(Variant, StringNameValueToStringReturnsSuppliedString)
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

TEST(Variant, StringNameValueToStringNameReturnsSuppliedStringName)
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

TEST(Variant, StringNameToVec3AlwaysReturnsZeroedVec)
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

TEST(Variant, HashOfStringNameVariantIsSameAsHashOfStringVariant)
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
        const auto snv = Variant{StringName{input}};
        const auto sv = Variant{std::string{input}};

        ASSERT_EQ(std::hash<Variant>{}(snv), std::hash<Variant>{}(sv));
    }
}

TEST(Variant, StringNameVariantComparesEqualToEquivalentStringVariant)
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
        const auto snv = Variant{StringName{input}};
        const auto sv = Variant{std::string{input}};
        ASSERT_EQ(snv, sv);
    }
}

TEST(Variant, StringNameVariantComparesEqualToEquivalentStringVariantReversed)
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
        const auto snv = Variant{StringName{input}};
        const auto sv = Variant{std::string{input}};
        ASSERT_EQ(sv, snv);  // reversed, compared to other test
    }
}
