#include <oscar_document/Variant.hpp>

#include <glm/vec3.hpp>
#include <gtest/gtest.h>
#include <oscar/Bindings/GlmHelpers.hpp>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/Cpp20Shims.hpp>
#include <oscar_document/StringName.hpp>

#include <charconv>
#include <stdexcept>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

using osc::Color;
using osc::StringName;
using osc::doc::Variant;
using osc::doc::VariantType;

namespace
{
    float ToFloatOrZero(std::string_view v)
    {
        // TODO: temporarily using `std::strof` here, rather than `std::from_chars` (C++17),
        // because MacOS (Catalina) and Ubuntu 20 don't support the latter (as of Oct 2023)
        // for floating-point values

        std::string s{v};
        size_t pos = 0;
        try
        {
            return std::stof(s, &pos);
        }
        catch (std::exception const&)
        {
            return 0.0f;
        }
    }

    int ToIntOrZero(std::string_view v)
    {
        int result{};
        auto [ptr, ec] = std::from_chars(v.data(), v.data() + v.size(), result);
        return ec == std::errc() ? result : 0;
    }
}

TEST(Variant, CanDefaultConstruct)
{
    static_assert(std::is_default_constructible_v<Variant>);
}

TEST(Variant, CanExplicitlyContructFromBool)
{
    Variant vfalse{false};
    ASSERT_EQ(vfalse.toBool(), false);
    Variant vtrue{true};
    ASSERT_EQ(vtrue.toBool(), true);

    ASSERT_EQ(vtrue.getType(), VariantType::Bool);
}

TEST(Variant, CanImplicitlyConstructFromBool)
{
    static_assert(std::is_convertible_v<bool, Variant>);
}

TEST(Variant, CanExplicitlyConstructFromColor)
{
    Variant v{Color::red()};
    ASSERT_EQ(v.toColor(), Color::red());
    ASSERT_EQ(v.getType(), VariantType::Color);
}

TEST(Variant, CanImplicitlyConstructFromColor)
{
    static_assert(std::is_convertible_v<Color, Variant>);
}

TEST(Variant, CanExplicityConstructFromFloat)
{
    Variant v{1.0f};
    ASSERT_EQ(v.toFloat(), 1.0f);
    ASSERT_EQ(v.getType(), VariantType::Float);
}

TEST(Variant, CanImplicitlyConstructFromFloat)
{
    static_assert(std::is_convertible_v<float, Variant>);
}

TEST(Variant, CanExplicitlyConstructFromInt)
{
    Variant v{5};
    ASSERT_EQ(v.toInt(), 5);
    ASSERT_EQ(v.getType(), VariantType::Int);
}

TEST(Variant, CanImplicitlyConstructFromInt)
{
    static_assert(std::is_convertible_v<int, Variant>);
}

TEST(Variant, CanExplicitlyConstructFromStringRValue)
{
    Variant v{std::string{"stringrval"}};
    ASSERT_EQ(v.toString(), "stringrval");
    ASSERT_EQ(v.getType(), VariantType::String);
}

TEST(Variant, CanImplicitlyConstructFromStringRValue)
{
    static_assert(std::is_convertible_v<std::string&&, Variant>);
}

TEST(Variant, CanExplicitlyConstructFromStringLiteral)
{
    Variant v{"cstringliteral"};
    ASSERT_EQ(v.toString(), "cstringliteral");
    ASSERT_EQ(v.getType(), VariantType::String);
}

TEST(Variant, CanImplicitlyConstructFromStringLiteral)
{
    static_assert(std::is_convertible_v<decltype(""), Variant>);
}

TEST(Variant, CanExplicitlyConstructFromCStringView)
{
    Variant v{osc::CStringView{"cstringview"}};
    ASSERT_EQ(v.toString(), "cstringview");
    ASSERT_EQ(v.getType(), VariantType::String);
}

TEST(Variant, CanImplicitlyConstructFromCStringView)
{
    static_assert(std::is_convertible_v<osc::CStringView, Variant>);
}

TEST(Variant, CanExplicitlyConstructFromVec3)
{
    Variant v{glm::vec3{1.0f, 2.0f, 3.0f}};
    ASSERT_EQ(v.toVec3(), glm::vec3(1.0f, 2.0f, 3.0f));
    ASSERT_EQ(v.getType(), VariantType::Vec3);
}

TEST(Variant, CanImplicitlyConstructFromVec3)
{
    static_assert(std::is_convertible_v<glm::vec3, Variant>);
}

TEST(Variant, DefaultConstructedValueIsNil)
{
    ASSERT_EQ(Variant{}.getType(), VariantType::Nil);
}

TEST(Variant, NilValueToBoolReturnsFalse)
{
    ASSERT_EQ(Variant{}.toBool(), false);
}

TEST(Variant, NilValueToColorReturnsBlack)
{
    ASSERT_EQ(Variant{}.toColor(), Color::black());
}

TEST(Variant, NilValueToFloatReturnsZero)
{
    ASSERT_EQ(Variant{}.toFloat(), 0.0f);
}

TEST(Variant, NilValueToIntReturnsZero)
{
    ASSERT_EQ(Variant{}.toInt(), 0);
}

TEST(Variant, NilValueToStringReturnsNull)
{
    ASSERT_EQ(Variant{}.toString(), "<null>");
}

TEST(Variant, NilValueToStringNameReturnsEmptyStringName)
{
    ASSERT_EQ(Variant{}.toStringName(), StringName{});
}

TEST(Variant, NilValueToVec3ReturnsZeroedVec3)
{
    ASSERT_EQ(Variant{}.toVec3(), glm::vec3{});
}

TEST(Variant, BoolValueToBoolReturnsExpectedBools)
{
    ASSERT_EQ(Variant(false).toBool(), false);
    ASSERT_EQ(Variant(true).toBool(), true);
}

TEST(Variant, BoolValueToColorReturnsExpectedColors)
{
    ASSERT_EQ(Variant(false).toColor(), Color::black());
    ASSERT_EQ(Variant(true).toColor(), Color::white());
}

TEST(Variant, BoolValueToFloatReturnsExpectedFloats)
{
    ASSERT_EQ(Variant(false).toFloat(), 0.0f);
    ASSERT_EQ(Variant(true).toFloat(), 1.0f);
}

TEST(Variant, BoolValueToIntReturnsExpectedInts)
{
    ASSERT_EQ(Variant(false).toInt(), 0);
    ASSERT_EQ(Variant(true).toInt(), 1);
}

TEST(Variant, BoolValueToStringReturnsExpectedStrings)
{
    Variant vfalse{false};
    ASSERT_EQ(vfalse.toString(), "false");
    Variant vtrue{true};
    ASSERT_EQ(vtrue.toString(), "true");
}

TEST(Variant, BoolValueToStringNameReturnsEmptyStringName)
{
    ASSERT_EQ(Variant{false}.toStringName(), StringName{});
    ASSERT_EQ(Variant{true}.toStringName(), StringName{});
}

TEST(Variant, BoolValueToVec3ReturnsExpectedVectors)
{
    ASSERT_EQ(Variant(false).toVec3(), glm::vec3{});
    ASSERT_EQ(Variant(true).toVec3(), glm::vec3(1.0f, 1.0f, 1.0f));
}

TEST(Variant, ColorToBoolReturnsExpectedValues)
{
    ASSERT_EQ(Variant(Color::black()).toBool(), false);
    ASSERT_EQ(Variant(Color::white()).toBool(), true);
    ASSERT_EQ(Variant(Color::magenta()).toBool(), true);
}

TEST(Variant, ColorToColorReturnsExpectedValues)
{
    ASSERT_EQ(Variant(Color::black()).toColor(), Color::black());
    ASSERT_EQ(Variant(Color::red()).toColor(), Color::red());
    ASSERT_EQ(Variant(Color::yellow()).toColor(), Color::yellow());
}

TEST(Variant, ColorToFloatReturnsExpectedValues)
{
    // should only extract first channel, to match vec3 behavior for conversion
    ASSERT_EQ(Variant(Color::black()).toFloat(), 0.0f);
    ASSERT_EQ(Variant(Color::white()).toFloat(), 1.0f);
    ASSERT_EQ(Variant(Color::blue()).toFloat(), 0.0f);
}

TEST(Variant, ColorToIntReturnsExpectedValues)
{
    // should only extract first channel, to match vec3 behavior for conversion
    ASSERT_EQ(Variant(Color::black()).toInt(), 0);
    ASSERT_EQ(Variant(Color::white()).toInt(), 1);
    ASSERT_EQ(Variant(Color::cyan()).toInt(), 0);
    ASSERT_EQ(Variant(Color::yellow()).toInt(), 1);
}

TEST(Variant, ColorValueToStringReturnsSameAsToHtmlStringRGBA)
{
    auto const colors = osc::to_array(
    {
        Color::red(),
        Color::magenta(),
    });

    for (auto const& color : colors)
    {
        ASSERT_EQ(Variant(color).toString(), osc::ToHtmlStringRGBA(color));
    }
}

TEST(Variant, ColorValueToStringReturnsExpectedManualExamples)
{
    ASSERT_EQ(Variant{Color::yellow()}.toString(), "#ffff00ff");
    ASSERT_EQ(Variant{Color::magenta()}.toString(), "#ff00ffff");
}

TEST(Variant, ColorValueToVec3ReturnsFirst3Channels)
{
    ASSERT_EQ(Variant(Color(1.0f, 2.0f, 3.0f)).toVec3(), glm::vec3(1.0f, 2.0f, 3.0f));
    ASSERT_EQ(Variant(Color::red()).toVec3(), glm::vec3(1.0f, 0.0f, 0.0f));
}

TEST(Variant, FloatValueToBoolReturnsExpectedValues)
{
    ASSERT_EQ(Variant(0.0f).toBool(), false);
    ASSERT_EQ(Variant(-0.5f).toBool(), true);
    ASSERT_EQ(Variant(-1.0f).toBool(), true);
    ASSERT_EQ(Variant(1.0f).toBool(), true);
    ASSERT_EQ(Variant(0.75f).toBool(), true);
}

TEST(Variant, FloatValueToColorReturnsExpectedColor)
{
    for (float v : osc::to_array<float>({0.0f, 0.5f, 0.75, 1.0f}))
    {
        Color const expected = {v, v, v};
        ASSERT_EQ(Variant(v).toColor(), expected);
    }
}

TEST(Variant, FloatValueToFloatReturnsInput)
{
    ASSERT_EQ(Variant(0.0f).toFloat(), 0.0f);
    ASSERT_EQ(Variant(0.12345f).toFloat(), 0.12345f);
    ASSERT_EQ(Variant(-0.54321f).toFloat(), -0.54321f);
}

TEST(Variant, FloatValueToIntReturnsCastedResult)
{
    for (float v : osc::to_array<float>({-0.5f, -0.123f, 0.0f, 1.0f, 1337.0f}))
    {
        int const expected = static_cast<int>(v);
        ASSERT_EQ(Variant(v).toInt(), expected);
    }
}

TEST(Variant, FloatValueToStringReturnsToStringedResult)
{
    for (float v : osc::to_array<float>({-5.35f, -2.0f, -1.0f, 0.0f, 0.123f, 18000.0f}))
    {
        std::string const expected = std::to_string(v);
        ASSERT_EQ(Variant(v).toString(), expected);
    }
}

TEST(Variant, FloatValueToStringNameReturnsEmptyStringName)
{
    ASSERT_EQ(Variant{0.0f}.toStringName(), StringName{});
    ASSERT_EQ(Variant{1.0f}.toStringName(), StringName{});
}

TEST(Variant, FloatValueToVec3ReturnsVec3FilledWithFloat)
{
    for (float v : osc::to_array<float>({-20000.0f, -5.328f, -1.2f, 0.0f, 0.123f, 50.0f, 18000.0f}))
    {
        glm::vec3 const expected = {v, v, v};
        ASSERT_EQ(Variant(v).toVec3(), expected);
    }
}

TEST(Variant, IntValueToBoolReturnsExpectedResults)
{
    ASSERT_EQ(Variant(0).toBool(), false);
    ASSERT_EQ(Variant(1).toBool(), true);
    ASSERT_EQ(Variant(-1).toBool(), true);
    ASSERT_EQ(Variant(234056).toBool(), true);
    ASSERT_EQ(Variant(-12938).toBool(), true);
}

TEST(Variant, IntValueToColorReturnsBlackOrWhite)
{
    ASSERT_EQ(Variant(0).toColor(), Color::black());
    ASSERT_EQ(Variant(1).toColor(), Color::white());
    ASSERT_EQ(Variant(-1).toColor(), Color::white());
    ASSERT_EQ(Variant(-230244).toColor(), Color::white());
    ASSERT_EQ(Variant(100983).toColor(), Color::white());
}

TEST(Variant, IntValueToFloatReturnsIntCastedToFloat)
{
    for (int v : osc::to_array<int>({-10000, -1000, -1, 0, 1, 17, 23000}))
    {
        float const expected = static_cast<float>(v);
        ASSERT_EQ(Variant(v).toFloat(), expected);
    }
}

TEST(Variant, IntValueToIntReturnsTheSuppliedInt)
{
    for (int v : osc::to_array<int>({ -123028, -2381, -32, -2, 0, 1, 1488, 5098}))
    {
        ASSERT_EQ(Variant(v).toInt(), v);
    }
}

TEST(Variant, IntValueToStringReturnsStringifiedInt)
{
    for (int v : osc::to_array<int>({ -121010, -13482, -1923, -123, -92, -7, 0, 1, 1294, 1209849}))
    {
        std::string const expected = std::to_string(v);
        ASSERT_EQ(Variant(v).toString(), expected);
    }
}

TEST(Variant, IntValueToStringNameReturnsEmptyString)
{
    ASSERT_EQ(Variant{-1}.toStringName(), StringName{});
    ASSERT_EQ(Variant{0}.toStringName(), StringName{});
    ASSERT_EQ(Variant{1337}.toStringName(), StringName{});
}

TEST(Variant, IntValueToVec3CastsValueToFloatThenPlacesInAllSlots)
{
    for (int v : osc::to_array<int>({ -12193, -1212, -738, -12, -1, 0, 1, 18, 1294, 1209849}))
    {
        float const vf = static_cast<float>(v);
        glm::vec3 const expected = {vf, vf, vf};
        ASSERT_EQ(Variant(v).toVec3(), expected);
    }
}

TEST(Variant, StringValueToBoolReturnsExpectedBoolValues)
{
    ASSERT_EQ(Variant("false").toBool(), false);
    ASSERT_EQ(Variant("FALSE").toBool(), false);
    ASSERT_EQ(Variant("False").toBool(), false);
    ASSERT_EQ(Variant("FaLsE").toBool(), false);
    ASSERT_EQ(Variant("0").toBool(), false);
    ASSERT_EQ(Variant("").toBool(), false);

    // all other strings are effectively `true`
    ASSERT_EQ(Variant("true").toBool(), true);
    ASSERT_EQ(Variant("non-empty string").toBool(), true);
    ASSERT_EQ(Variant(" ").toBool(), true);
}

TEST(Variant, StringValueToColorWorksIfStringIsAValidHTMLColorString)
{
    ASSERT_EQ(Variant{"#ff0000ff"}.toColor(), Color::red());
    ASSERT_EQ(Variant{"#00ff00ff"}.toColor(), Color::green());
    ASSERT_EQ(Variant{"#ffffffff"}.toColor(), Color::white());
    ASSERT_EQ(Variant{"#00000000"}.toColor(), Color::clear());
    ASSERT_EQ(Variant{"#000000ff"}.toColor(), Color::black());
    ASSERT_EQ(Variant{"#000000FF"}.toColor(), Color::black());
    ASSERT_EQ(Variant{"#123456ae"}.toColor(), *osc::TryParseHtmlString("#123456ae"));
}

TEST(Variant, StringValueColorReturnsBlackIfStringIsInvalidHTMLColorString)
{
    ASSERT_EQ(Variant{"not a color"}.toColor(), Color::black());
}

TEST(Variant, StringValueToFloatTriesToParseStringAsFloatAndReturnsZeroOnFailure)
{
    auto const inputs = osc::to_array<std::string_view>(
    {
        "-1.0",
        "20e-10",
        "",
        "1",
        "1.0",
        "2.0",
        "not a number",
        "  ",
    });

    for (auto const& input : inputs)
    {
        float const expectedOutput = ToFloatOrZero(input);
        ASSERT_EQ(Variant{input}.toFloat(), expectedOutput);
    }
}

TEST(Variant, StringValueToIntTriesToParseStringAsBase10Int)
{
    auto const inputs = osc::to_array<std::string_view>(
    {
        "-1.0",
        "20e-10",
        "",
        "1",
        "1.0",
        "2.0",
        "not a number",
        "  ",
    });

    for (auto const& input : inputs)
    {
        int const expectedOutput = ToIntOrZero(input);
        ASSERT_EQ(Variant{input}.toInt(), expectedOutput);
    }
}

TEST(Variant, StringValueToStringReturnsSuppliedString)
{
    auto const inputs = osc::to_array<std::string_view>(
    {
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

    for (auto const& input : inputs)
    {
        ASSERT_EQ(Variant{input}.toString(), input);
    }
}

TEST(Variant, StringValueToStringNameReturnsSuppliedStringAsAStringName)
{
    auto const inputs = osc::to_array<std::string_view>(
    {
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

    for (auto const& input : inputs)
    {
        ASSERT_EQ(Variant{input}.toStringName(), StringName{input});
    }
}

TEST(Variant, StringValueToVec3AlwaysReturnsZeroedVec)
{
    auto const inputs = osc::to_array<std::string_view>(
    {
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

    for (auto const& input : inputs)
    {
        ASSERT_EQ(Variant{input}.toVec3(), glm::vec3{});
    }
}

TEST(Variant, Vec3ValueToBoolReturnsFalseForZeroVec)
{
    ASSERT_EQ(Variant{glm::vec3{}}.toBool(), false);
}

TEST(Variant, Vec3ValueToBoolReturnsFalseIfXIsZeroRegardlessOfOtherComponents)
{
    // why: because it's consistent with the `toInt()` and `toFloat()` behavior, and
    // one would logically expect `if (v.toInt())` to behave the same as `if (v.toBool())`
    ASSERT_EQ(Variant{glm::vec3{0.0f}}.toBool(), false);
    ASSERT_EQ(Variant{glm::vec3(0.0f, 0.0f, 1000.0f)}.toBool(), false);
    ASSERT_EQ(Variant{glm::vec3(0.0f, 7.0f, -30.0f)}.toBool(), false);
    ASSERT_EQ(Variant{glm::vec3(0.0f, 2.0f, 1.0f)}.toBool(), false);
    ASSERT_EQ(Variant{glm::vec3(0.0f, 1.0f, 1.0f)}.toBool(), false);
    ASSERT_EQ(Variant{glm::vec3(0.0f, -1.0f, 0.0f)}.toBool(), false);
    static_assert(+0.0f == -0.0f);
    ASSERT_EQ(Variant{glm::vec3(-0.0f, 0.0f, 1000.0f)}.toBool(), false);  // how fun ;)
}

TEST(Variant, Vec3ValueToBoolReturnsTrueIfXIsNonZeroRegardlessOfOtherComponents)
{
    ASSERT_EQ(Variant{glm::vec3{1.0f}}.toBool(), true);
    ASSERT_EQ(Variant{glm::vec3(2.0f, 7.0f, -30.0f)}.toBool(), true);
    ASSERT_EQ(Variant{glm::vec3(30.0f, 2.0f, 1.0f)}.toBool(), true);
    ASSERT_EQ(Variant{glm::vec3(-40.0f, 1.0f, 1.0f)}.toBool(), true);
    ASSERT_EQ(Variant{glm::vec3(std::numeric_limits<float>::quiet_NaN(), -1.0f, 0.0f)}.toBool(), true);
}

TEST(Variant, Vec3ValueToColorExtractsTheElementsIntoRGB)
{
    auto const testCases = osc::to_array<glm::vec3>(
    {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 1.0f},
        {-1.0f, 0.0f, 1.0f},
        {10.0f, 0.0f, 7.5f},
        {0.0f, -20.0f, 0.5f},
    });

    for (auto const& testCase : testCases)
    {
        ASSERT_EQ(Variant{testCase}.toColor(), Color{testCase});
    }
}

TEST(Variant, Vec3ValueToFloatExtractsXToTheFloat)
{
    auto const testCases = osc::to_array<glm::vec3>(
    {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 1.0f},
        {-1.0f, 0.0f, 1.0f},
        {10.0f, 0.0f, 7.5f},
        {0.0f, -20.0f, 0.5f},
    });

    for (auto const& testCase : testCases)
    {
        ASSERT_EQ(Variant{testCase}.toFloat(), testCase.x);
    }
}

TEST(Variant, Vec3ValueToIntExtractsXToTheInt)
{
    auto const testCases = osc::to_array<glm::vec3>(
    {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 1.0f},
        {-1.0f, 0.0f, 1.0f},
        {10.0f, 0.0f, 7.5f},
        {0.0f, -20.0f, 0.5f},
    });

    for (auto const& testCase : testCases)
    {
        ASSERT_EQ(Variant{testCase}.toInt(), static_cast<int>(testCase.x));
    }
}

TEST(Variant, Vec3ValueToStringReturnsSameAsDirectlyConvertingVectorToString)
{
    auto const testCases = osc::to_array<glm::vec3>(
    {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 1.0f},
        {-1.0f, 0.0f, 1.0f},
        {10.0f, 0.0f, 7.5f},
        {0.0f, -20.0f, 0.5f},
    });

    for (auto const& testCase : testCases)
    {
        ASSERT_EQ(Variant{testCase}.toString(), osc::to_string(testCase));
    }
}

TEST(Variant, Vec3ValueToStringNameReturnsAnEmptyString)
{
    ASSERT_EQ(Variant{glm::vec3{}}.toStringName(), StringName{});
    ASSERT_EQ(Variant{glm::vec3(0.0f, -20.0f, 0.5f)}.toStringName(), StringName{});
}

TEST(Variant, Vec3ValueToVec3ReturnsOriginalValue)
{
    auto const testCases = osc::to_array<glm::vec3>(
    {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 1.0f},
        {-1.0f, 0.0f, 1.0f},
        {10.0f, 0.0f, 7.5f},
        {0.0f, -20.0f, 0.5f},
    });

    for (auto const& testCase : testCases)
    {
        ASSERT_EQ(Variant{testCase}.toVec3(), testCase);
    }
}

TEST(Variant, IsAlwaysEqualToACopyOfItself)
{
    auto const testCases = osc::to_array<Variant>(
    {
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
        Variant{osc::StringName{"a string name"}},
        Variant{glm::vec3{}},
        Variant{glm::vec3{1.0f}},
        Variant{glm::vec3{-1.0f}},
        Variant{glm::vec3{0.5f}},
        Variant{glm::vec3{-0.5f}},
    });

    for (auto const& tc : testCases)
    {
        ASSERT_EQ(tc, tc) << "input: " << tc.toString();
    }

    auto const exceptions = osc::to_array<Variant>(
    {
        Variant{std::numeric_limits<float>::quiet_NaN()},
        Variant{std::numeric_limits<float>::signaling_NaN()},
    });
    for (auto const& tc : exceptions)
    {
        ASSERT_NE(tc, tc) << "input: " << tc.toString();
    }
}

TEST(Variant, IsNotEqualToOtherValuesEvenIfConversionIsPossible)
{
    auto const testCases = osc::to_array<Variant>(
    {
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
        Variant{osc::StringName{"a stringname can be compared to a string, though"}},
        Variant{glm::vec3{}},
        Variant{glm::vec3{1.0f}},
        Variant{glm::vec3{-1.0f}},
        Variant{glm::vec3{0.5f}},
        Variant{glm::vec3{-0.5f}},
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
    auto const testCases = osc::to_array<Variant>(
    {
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
        Variant{osc::StringName{"a string name"}},
        Variant{glm::vec3{}},
        Variant{glm::vec3{1.0f}},
        Variant{glm::vec3{-1.0f}},
        Variant{glm::vec3{0.5f}},
        Variant{glm::vec3{-0.5f}},
    });

    for (auto const& testCase : testCases)
    {
        ASSERT_NO_THROW({ std::hash<Variant>{}(testCase); });
    }
}

TEST(Variant, HashesForStringValuesMatchStdStringEtc)
{
    auto const strings = osc::to_array<std::string_view>(
    {
        "false",
        "true",
        "0",
        "1",
        "a string",
    });

    for (auto const& s : strings)
    {
        Variant const variant{s};
        auto const hash = std::hash<Variant>{}(variant);

        ASSERT_EQ(hash, std::hash<std::string>{}(std::string{s}));
        ASSERT_EQ(hash, std::hash<std::string_view>{}(s));
        ASSERT_EQ(hash, std::hash<osc::CStringView>{}(std::string{s}));
    }
}

TEST(Variant, ConstructingFromstringNameMakesGetTypeReturnStringNameType)
{
    ASSERT_EQ(Variant(osc::StringName{"s"}).getType(), VariantType::StringName);
}

TEST(Variant, ConstructedFromSameStringNameComparesEquivalent)
{
    ASSERT_EQ(Variant{osc::StringName{"string"}}, Variant{osc::StringName{"string"}});
}

TEST(Variant, ConstructedFromStringNameComparesInequivalentToVariantConstructedFromDifferentString)
{
    ASSERT_NE(Variant{osc::StringName{"a"}}, Variant{std::string{"b"}});
}

TEST(Variant, StringNameValueToBoolReturnsExpectedBoolValues)
{
    ASSERT_EQ(Variant(osc::StringName{"false"}).toBool(), false);
    ASSERT_EQ(Variant(osc::StringName{"FALSE"}).toBool(), false);
    ASSERT_EQ(Variant(osc::StringName{"False"}).toBool(), false);
    ASSERT_EQ(Variant(osc::StringName{"FaLsE"}).toBool(), false);
    ASSERT_EQ(Variant(osc::StringName{"0"}).toBool(), false);
    ASSERT_EQ(Variant(osc::StringName{""}).toBool(), false);

    // all other strings are effectively `true`
    ASSERT_EQ(Variant(osc::StringName{"true"}).toBool(), true);
    ASSERT_EQ(Variant(osc::StringName{"non-empty string"}).toBool(), true);
    ASSERT_EQ(Variant(osc::StringName{" "}).toBool(), true);
}

TEST(Variant, StringNameValueToColorWorksIfStringIsAValidHTMLColorString)
{
    ASSERT_EQ(Variant{osc::StringName{"#ff0000ff"}}.toColor(), Color::red());
    ASSERT_EQ(Variant{osc::StringName{"#00ff00ff"}}.toColor(), Color::green());
    ASSERT_EQ(Variant{osc::StringName{"#ffffffff"}}.toColor(), Color::white());
    ASSERT_EQ(Variant{osc::StringName{"#00000000"}}.toColor(), Color::clear());
    ASSERT_EQ(Variant{osc::StringName{"#000000ff"}}.toColor(), Color::black());
    ASSERT_EQ(Variant{osc::StringName{"#000000FF"}}.toColor(), Color::black());
    ASSERT_EQ(Variant{osc::StringName{"#123456ae"}}.toColor(), *osc::TryParseHtmlString("#123456ae"));
}

TEST(Variant, StringNameValueToColorReturnsBlackIfStringIsInvalidHTMLColorString)
{
    ASSERT_EQ(Variant{osc::StringName{"not a color"}}.toColor(), Color::black());
}

TEST(Variant, StringNameValueToFloatTriesToParseStringAsFloatAndReturnsZeroOnFailure)
{
    auto const inputs = osc::to_array<std::string_view>(
    {
        "-1.0",
        "20e-10",
        "",
        "1",
        "1.0",
        "2.0",
        "not a number",
        "  ",
    });

    for (auto const& input : inputs)
    {
        float const expectedOutput = ToFloatOrZero(input);
        ASSERT_EQ(Variant{osc::StringName{input}}.toFloat(), expectedOutput);
    }
}

TEST(Variant, StringNameValueToIntTriesToParseStringAsBase10Int)
{
    auto const inputs = osc::to_array<std::string_view>(
    {
        "-1.0",
        "20e-10",
        "",
        "1",
        "1.0",
        "2.0",
        "not a number",
        "  ",
    });

    for (auto const& input : inputs)
    {
        int const expectedOutput = ToIntOrZero(input);
        ASSERT_EQ(Variant{osc::StringName{input}}.toInt(), expectedOutput);
    }
}

TEST(Variant, StringNameValueToStringReturnsSuppliedString)
{
    auto const inputs = osc::to_array<std::string_view>(
    {
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

    for (auto const& input : inputs)
    {
        ASSERT_EQ(Variant{osc::StringName{input}}.toString(), input);
    }
}

TEST(Variant, StringNameValueToStringNameReturnsSuppliedStringName)
{
    auto const inputs = osc::to_array<std::string_view>(
    {
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

    for (auto const& input : inputs)
    {
        ASSERT_EQ(Variant{StringName{input}}.toStringName(), StringName{input});
    }
}

TEST(Variant, StringNameToVec3AlwaysReturnsZeroedVec)
{
    auto const inputs = osc::to_array<std::string_view>(
    {
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

    for (auto const& input : inputs)
    {
        ASSERT_EQ(Variant{osc::StringName{input}}.toVec3(), glm::vec3{});
    }
}

TEST(Variant, HashOfStringNameVariantIsSameAsHashOfStringVariant)
{
    auto const inputs = osc::to_array<std::string_view>(
    {
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

    for (auto const& input : inputs)
    {
        auto snv = Variant{osc::StringName{input}};
        auto sv = Variant{std::string{input}};

        ASSERT_EQ(std::hash<Variant>{}(snv), std::hash<Variant>{}(sv));
    }
}

TEST(Variant, StringNameVariantComparesEqualToEquivalentStringVariant)
{
    auto const inputs = osc::to_array<std::string_view>(
    {
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

    for (auto const& input : inputs)
    {
        auto snv = Variant{StringName{input}};
        auto sv = Variant{std::string{input}};
        ASSERT_EQ(snv, sv);
    }
}

TEST(Variant, StringNameVariantComparesEqualToEquivalentStringVariantReversed)
{
    auto const inputs = osc::to_array<std::string_view>(
    {
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

    for (auto const& input : inputs)
    {
        auto snv = Variant{StringName{input}};
        auto sv = Variant{std::string{input}};
        ASSERT_EQ(sv, snv);  // reversed, compared to other test
    }
}
