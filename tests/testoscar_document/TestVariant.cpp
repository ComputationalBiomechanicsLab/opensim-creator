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
#include <utility>

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

TEST(Variant, CanExplicitlyContructFromBool)
{
    osc::doc::Variant vfalse{false};
    ASSERT_EQ(vfalse.toBool(), false);
    osc::doc::Variant vtrue{true};
    ASSERT_EQ(vtrue.toBool(), true);

    ASSERT_EQ(vtrue.getType(), osc::doc::VariantType::Bool);
}

TEST(Variant, CanExplicitlyConstructFromColor)
{
    osc::doc::Variant v{osc::Color::red()};
    ASSERT_EQ(v.toColor(), osc::Color::red());
    ASSERT_EQ(v.getType(), osc::doc::VariantType::Color);
}

TEST(Variant, CanExplicityConstructFromFloat)
{
    osc::doc::Variant v{1.0f};
    ASSERT_EQ(v.toFloat(), 1.0f);
    ASSERT_EQ(v.getType(), osc::doc::VariantType::Float);
}

TEST(Variant, CanExplicitlyConstructFromInt)
{
    osc::doc::Variant v{5};
    ASSERT_EQ(v.toInt(), 5);
    ASSERT_EQ(v.getType(), osc::doc::VariantType::Int);
}

TEST(Variant, CanExplicitlyConstructFromStringRValue)
{
    osc::doc::Variant v{std::string{"stringrval"}};
    ASSERT_EQ(v.toString(), "stringrval");
    ASSERT_EQ(v.getType(), osc::doc::VariantType::String);
}

TEST(Variant, CanExplicitlyConstructFromStringLiteral)
{
    osc::doc::Variant v{"cstringliteral"};
    ASSERT_EQ(v.toString(), "cstringliteral");
    ASSERT_EQ(v.getType(), osc::doc::VariantType::String);
}

TEST(Variant, CanExplicitlyConstructFromCStringView)
{
    osc::doc::Variant v{osc::CStringView{"cstringview"}};
    ASSERT_EQ(v.toString(), "cstringview");
    ASSERT_EQ(v.getType(), osc::doc::VariantType::String);
}

TEST(Variant, CanExplicitlyConstructFromVec3)
{
    osc::doc::Variant v{glm::vec3{1.0f, 2.0f, 3.0f}};
    ASSERT_EQ(v.toVec3(), glm::vec3(1.0f, 2.0f, 3.0f));
    ASSERT_EQ(v.getType(), osc::doc::VariantType::Vec3);
}

TEST(Variant, BoolValueToBoolReturnsExpectedBools)
{
    ASSERT_EQ(osc::doc::Variant(false).toBool(), false);
    ASSERT_EQ(osc::doc::Variant(true).toBool(), true);
}

TEST(Variant, BoolValueToColorReturnsExpectedColors)
{
    ASSERT_EQ(osc::doc::Variant(false).toColor(), osc::Color::black());
    ASSERT_EQ(osc::doc::Variant(true).toColor(), osc::Color::white());
}

TEST(Variant, BoolValueToFloatReturnsExpectedFloats)
{
    ASSERT_EQ(osc::doc::Variant(false).toFloat(), 0.0f);
    ASSERT_EQ(osc::doc::Variant(true).toFloat(), 1.0f);
}

TEST(Variant, BoolValueToIntReturnsExpectedInts)
{
    ASSERT_EQ(osc::doc::Variant(false).toInt(), 0);
    ASSERT_EQ(osc::doc::Variant(true).toInt(), 1);
}

TEST(Variant, BoolValueToStringReturnsExpectedStrings)
{
    osc::doc::Variant vfalse{false};
    ASSERT_EQ(vfalse.toString(), "false");
    osc::doc::Variant vtrue{true};
    ASSERT_EQ(vtrue.toString(), "true");
}

TEST(Variant, BoolValueToVec3ReturnsExpectedVectors)
{
    ASSERT_EQ(osc::doc::Variant(false).toVec3(), glm::vec3{});
    ASSERT_EQ(osc::doc::Variant(true).toVec3(), glm::vec3(1.0f, 1.0f, 1.0f));
}

TEST(Variant, ColorToBoolReturnsExpectedValues)
{
    ASSERT_EQ(osc::doc::Variant(osc::Color::black()).toBool(), false);
    ASSERT_EQ(osc::doc::Variant(osc::Color::white()).toBool(), true);
    ASSERT_EQ(osc::doc::Variant(osc::Color::magenta()).toBool(), true);
}

TEST(Variant, ColorToColorReturnsExpectedValues)
{
    ASSERT_EQ(osc::doc::Variant(osc::Color::black()).toColor(), osc::Color::black());
    ASSERT_EQ(osc::doc::Variant(osc::Color::red()).toColor(), osc::Color::red());
    ASSERT_EQ(osc::doc::Variant(osc::Color::yellow()).toColor(), osc::Color::yellow());
}

TEST(Variant, ColorToFloatReturnsExpectedValues)
{
    // should only extract first channel, to match vec3 behavior for conversion
    ASSERT_EQ(osc::doc::Variant(osc::Color::black()).toFloat(), 0.0f);
    ASSERT_EQ(osc::doc::Variant(osc::Color::white()).toFloat(), 1.0f);
    ASSERT_EQ(osc::doc::Variant(osc::Color::blue()).toFloat(), 0.0f);
}

TEST(Variant, ColorToIntReturnsExpectedValues)
{
    // should only extract first channel, to match vec3 behavior for conversion
    ASSERT_EQ(osc::doc::Variant(osc::Color::black()).toInt(), 0);
    ASSERT_EQ(osc::doc::Variant(osc::Color::white()).toInt(), 1);
    ASSERT_EQ(osc::doc::Variant(osc::Color::cyan()).toInt(), 0);
    ASSERT_EQ(osc::doc::Variant(osc::Color::yellow()).toInt(), 1);
}

TEST(Variant, ColorValueToStringReturnsSameAsToHtmlStringRGBA)
{
    auto const colors = osc::to_array(
    {
        osc::Color::red(),
        osc::Color::magenta(),
    });

    for (auto const& color : colors)
    {
        ASSERT_EQ(osc::doc::Variant(color).toString(), osc::ToHtmlStringRGBA(color));
    }
}

TEST(Variant, ColorValueToStringReturnsExpectedManualExamples)
{
    ASSERT_EQ(osc::doc::Variant{osc::Color::yellow()}.toString(), "#ffff00ff");
    ASSERT_EQ(osc::doc::Variant{osc::Color::magenta()}.toString(), "#ff00ffff");
}

TEST(Variant, ColorValueToVec3ReturnsFirst3Channels)
{
    ASSERT_EQ(osc::doc::Variant(osc::Color(1.0f, 2.0f, 3.0f)).toVec3(), glm::vec3(1.0f, 2.0f, 3.0f));
    ASSERT_EQ(osc::doc::Variant(osc::Color::red()).toVec3(), glm::vec3(1.0f, 0.0f, 0.0f));
}

TEST(Variant, FloatValueToBoolReturnsExpectedValues)
{
    ASSERT_EQ(osc::doc::Variant(0.0f).toBool(), false);
    ASSERT_EQ(osc::doc::Variant(-0.5f).toBool(), true);
    ASSERT_EQ(osc::doc::Variant(-1.0f).toBool(), true);
    ASSERT_EQ(osc::doc::Variant(1.0f).toBool(), true);
    ASSERT_EQ(osc::doc::Variant(0.75f).toBool(), true);
}

TEST(Variant, FloatValueToColorReturnsExpectedColor)
{
    for (float v : osc::to_array<float>({0.0f, 0.5f, 0.75, 1.0f}))
    {
        osc::Color const expected = {v, v, v};
        ASSERT_EQ(osc::doc::Variant(v).toColor(), expected);
    }
}

TEST(Variant, FloatValueToFloatReturnsInput)
{
    ASSERT_EQ(osc::doc::Variant(0.0f).toFloat(), 0.0f);
    ASSERT_EQ(osc::doc::Variant(0.12345f).toFloat(), 0.12345f);
    ASSERT_EQ(osc::doc::Variant(-0.54321f).toFloat(), -0.54321f);
}

TEST(Variant, FloatValueToIntReturnsCastedResult)
{
    for (float v : osc::to_array<float>({-0.5f, -0.123f, 0.0f, 1.0f, 1337.0f}))
    {
        int const expected = static_cast<int>(v);
        ASSERT_EQ(osc::doc::Variant(v).toInt(), expected);
    }
}

TEST(Variant, FloatValueToStringReturnsToStringedResult)
{
    for (float v : osc::to_array<float>({-5.35f, -2.0f, -1.0f, 0.0f, 0.123f, 18000.0f}))
    {
        std::string const expected = std::to_string(v);
        ASSERT_EQ(osc::doc::Variant(v).toString(), expected);
    }
}

TEST(Variant, FloatValueToVec3ReturnsVec3FilledWithFloat)
{
    for (float v : osc::to_array<float>({-20000.0f, -5.328f, -1.2f, 0.0f, 0.123f, 50.0f, 18000.0f}))
    {
        glm::vec3 const expected = {v, v, v};
        ASSERT_EQ(osc::doc::Variant(v).toVec3(), expected);
    }
}

TEST(Variant, IntValueToBoolReturnsExpectedResults)
{
    ASSERT_EQ(osc::doc::Variant(0).toBool(), false);
    ASSERT_EQ(osc::doc::Variant(1).toBool(), true);
    ASSERT_EQ(osc::doc::Variant(-1).toBool(), true);
    ASSERT_EQ(osc::doc::Variant(234056).toBool(), true);
    ASSERT_EQ(osc::doc::Variant(-12938).toBool(), true);
}

TEST(Variant, IntValueToColorReturnsBlackOrWhite)
{
    ASSERT_EQ(osc::doc::Variant(0).toColor(), osc::Color::black());
    ASSERT_EQ(osc::doc::Variant(1).toColor(), osc::Color::white());
    ASSERT_EQ(osc::doc::Variant(-1).toColor(), osc::Color::white());
    ASSERT_EQ(osc::doc::Variant(-230244).toColor(), osc::Color::white());
    ASSERT_EQ(osc::doc::Variant(100983).toColor(), osc::Color::white());
}

TEST(Variant, IntValueToFloatReturnsIntCastedToFloat)
{
    for (int v : osc::to_array<int>({-10000, -1000, -1, 0, 1, 17, 23000}))
    {
        float const expected = static_cast<float>(v);
        ASSERT_EQ(osc::doc::Variant(v).toFloat(), expected);
    }
}

TEST(Variant, IntValueToIntReturnsTheSuppliedInt)
{
    for (int v : osc::to_array<int>({ -123028, -2381, -32, -2, 0, 1, 1488, 5098}))
    {
        ASSERT_EQ(osc::doc::Variant(v).toInt(), v);
    }
}

TEST(Variant, IntValueToStringReturnsStringifiedInt)
{
    for (int v : osc::to_array<int>({ -121010, -13482, -1923, -123, -92, -7, 0, 1, 1294, 1209849}))
    {
        std::string const expected = std::to_string(v);
        ASSERT_EQ(osc::doc::Variant(v).toString(), expected);
    }
}

TEST(Variant, IntValueToVec3CastsValueToFloatThenPlacesInAllSlots)
{
    for (int v : osc::to_array<int>({ -12193, -1212, -738, -12, -1, 0, 1, 18, 1294, 1209849}))
    {
        float const vf = static_cast<float>(v);
        glm::vec3 const expected = {vf, vf, vf};
        ASSERT_EQ(osc::doc::Variant(v).toVec3(), expected);
    }
}

TEST(Variant, StringValueToBoolReturnsExpectedBoolValues)
{
    ASSERT_EQ(osc::doc::Variant("false").toBool(), false);
    ASSERT_EQ(osc::doc::Variant("FALSE").toBool(), false);
    ASSERT_EQ(osc::doc::Variant("False").toBool(), false);
    ASSERT_EQ(osc::doc::Variant("FaLsE").toBool(), false);
    ASSERT_EQ(osc::doc::Variant("0").toBool(), false);
    ASSERT_EQ(osc::doc::Variant("").toBool(), false);

    // all other strings are effectively `true`
    ASSERT_EQ(osc::doc::Variant("true").toBool(), true);
    ASSERT_EQ(osc::doc::Variant("non-empty string").toBool(), true);
    ASSERT_EQ(osc::doc::Variant(" ").toBool(), true);
}

TEST(Variant, StringValueToColorWorksIfStringIsAValidHTMLColorString)
{
    ASSERT_EQ(osc::doc::Variant{"#ff0000ff"}.toColor(), osc::Color::red());
    ASSERT_EQ(osc::doc::Variant{"#00ff00ff"}.toColor(), osc::Color::green());
    ASSERT_EQ(osc::doc::Variant{"#ffffffff"}.toColor(), osc::Color::white());
    ASSERT_EQ(osc::doc::Variant{"#00000000"}.toColor(), osc::Color::clear());
    ASSERT_EQ(osc::doc::Variant{"#000000ff"}.toColor(), osc::Color::black());
    ASSERT_EQ(osc::doc::Variant{"#000000FF"}.toColor(), osc::Color::black());
    ASSERT_EQ(osc::doc::Variant{"#123456ae"}.toColor(), *osc::TryParseHtmlString("#123456ae"));
}

TEST(Variant, StringValueColorReturnsBlackIfStringIsInvalidHTMLColorString)
{
    ASSERT_EQ(osc::doc::Variant{"not a color"}.toColor(), osc::Color::black());
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
        ASSERT_EQ(osc::doc::Variant{input}.toFloat(), expectedOutput);
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
        ASSERT_EQ(osc::doc::Variant{input}.toInt(), expectedOutput);
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
        ASSERT_EQ(osc::doc::Variant{input}.toString(), input);
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
        ASSERT_EQ(osc::doc::Variant{input}.toVec3(), glm::vec3{});
    }
}

TEST(Variant, Vec3ValueToBoolReturnsFalseForZeroVec)
{
    ASSERT_EQ(osc::doc::Variant{glm::vec3{}}.toBool(), false);
}

TEST(Variant, Vec3ValueToBoolReturnsFalseIfXIsZeroRegardlessOfOtherComponents)
{
    // why: because it's consistent with the `toInt()` and `toFloat()` behavior, and
    // one would logically expect `if (v.toInt())` to behave the same as `if (v.toBool())`
    ASSERT_EQ(osc::doc::Variant{glm::vec3{0.0f}}.toBool(), false);
    ASSERT_EQ(osc::doc::Variant{glm::vec3(0.0f, 0.0f, 1000.0f)}.toBool(), false);
    ASSERT_EQ(osc::doc::Variant{glm::vec3(0.0f, 7.0f, -30.0f)}.toBool(), false);
    ASSERT_EQ(osc::doc::Variant{glm::vec3(0.0f, 2.0f, 1.0f)}.toBool(), false);
    ASSERT_EQ(osc::doc::Variant{glm::vec3(0.0f, 1.0f, 1.0f)}.toBool(), false);
    ASSERT_EQ(osc::doc::Variant{glm::vec3(0.0f, -1.0f, 0.0f)}.toBool(), false);
    static_assert(+0.0f == -0.0f);
    ASSERT_EQ(osc::doc::Variant{glm::vec3(-0.0f, 0.0f, 1000.0f)}.toBool(), false);  // how fun ;)
}

TEST(Variant, Vec3ValueToBoolReturnsTrueIfXIsNonZeroRegardlessOfOtherComponents)
{
    ASSERT_EQ(osc::doc::Variant{glm::vec3{1.0f}}.toBool(), true);
    ASSERT_EQ(osc::doc::Variant{glm::vec3(2.0f, 7.0f, -30.0f)}.toBool(), true);
    ASSERT_EQ(osc::doc::Variant{glm::vec3(30.0f, 2.0f, 1.0f)}.toBool(), true);
    ASSERT_EQ(osc::doc::Variant{glm::vec3(-40.0f, 1.0f, 1.0f)}.toBool(), true);
    ASSERT_EQ(osc::doc::Variant{glm::vec3(std::numeric_limits<float>::quiet_NaN(), -1.0f, 0.0f)}.toBool(), true);
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
        ASSERT_EQ(osc::doc::Variant{testCase}.toColor(), osc::Color{testCase});
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
        ASSERT_EQ(osc::doc::Variant{testCase}.toFloat(), testCase.x);
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
        ASSERT_EQ(osc::doc::Variant{testCase}.toInt(), static_cast<int>(testCase.x));
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
        ASSERT_EQ(osc::doc::Variant{testCase}.toString(), osc::to_string(testCase));
    }
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
        ASSERT_EQ(osc::doc::Variant{testCase}.toVec3(), testCase);
    }
}

TEST(Variant, IsAlwaysEqualToACopyOfItself)
{
    auto const testCases = osc::to_array<osc::doc::Variant>(
    {
        osc::doc::Variant{false},
        osc::doc::Variant{true},
        osc::doc::Variant{osc::Color::white()},
        osc::doc::Variant{osc::Color::black()},
        osc::doc::Variant{osc::Color::clear()},
        osc::doc::Variant{osc::Color::magenta()},
        osc::doc::Variant{-1.0f},
        osc::doc::Variant{0.0f},
        osc::doc::Variant{-30.0f},
        osc::doc::Variant{std::numeric_limits<float>::infinity()},
        osc::doc::Variant{-std::numeric_limits<float>::infinity()},
        osc::doc::Variant{std::numeric_limits<int>::min()},
        osc::doc::Variant{std::numeric_limits<int>::max()},
        osc::doc::Variant{-1},
        osc::doc::Variant{0},
        osc::doc::Variant{1},
        osc::doc::Variant{""},
        osc::doc::Variant{"false"},
        osc::doc::Variant{"true"},
        osc::doc::Variant{"0"},
        osc::doc::Variant{"1"},
        osc::doc::Variant{"a string"},
        osc::doc::Variant{osc::StringName{"a string name"}},
        osc::doc::Variant{glm::vec3{}},
        osc::doc::Variant{glm::vec3{1.0f}},
        osc::doc::Variant{glm::vec3{-1.0f}},
        osc::doc::Variant{glm::vec3{0.5f}},
        osc::doc::Variant{glm::vec3{-0.5f}},
    });

    for (auto const& tc : testCases)
    {
        ASSERT_EQ(tc, tc) << "input: " << tc.toString();
    }

    auto const exceptions = osc::to_array<osc::doc::Variant>(
    {
        osc::doc::Variant{std::numeric_limits<float>::quiet_NaN()},
        osc::doc::Variant{std::numeric_limits<float>::signaling_NaN()},
    });
    for (auto const& tc : exceptions)
    {
        ASSERT_NE(tc, tc) << "input: " << tc.toString();
    }
}

TEST(Variant, IsNotEqualToOtherValuesEvenIfConversionIsPossible)
{
    auto const testCases = osc::to_array<osc::doc::Variant>(
    {
        osc::doc::Variant{false},
        osc::doc::Variant{true},
        osc::doc::Variant{osc::Color::white()},
        osc::doc::Variant{osc::Color::black()},
        osc::doc::Variant{osc::Color::clear()},
        osc::doc::Variant{osc::Color::magenta()},
        osc::doc::Variant{-1.0f},
        osc::doc::Variant{0.0f},
        osc::doc::Variant{-30.0f},
        osc::doc::Variant{std::numeric_limits<float>::quiet_NaN()},
        osc::doc::Variant{std::numeric_limits<float>::signaling_NaN()},
        osc::doc::Variant{std::numeric_limits<float>::infinity()},
        osc::doc::Variant{-std::numeric_limits<float>::infinity()},
        osc::doc::Variant{std::numeric_limits<int>::min()},
        osc::doc::Variant{std::numeric_limits<int>::max()},
        osc::doc::Variant{-1},
        osc::doc::Variant{0},
        osc::doc::Variant{1},
        osc::doc::Variant{""},
        osc::doc::Variant{"false"},
        osc::doc::Variant{"true"},
        osc::doc::Variant{"0"},
        osc::doc::Variant{"1"},
        osc::doc::Variant{"a string"},
        osc::doc::Variant{osc::StringName{"a stringname can be compared to a string, though"}},
        osc::doc::Variant{glm::vec3{}},
        osc::doc::Variant{glm::vec3{1.0f}},
        osc::doc::Variant{glm::vec3{-1.0f}},
        osc::doc::Variant{glm::vec3{0.5f}},
        osc::doc::Variant{glm::vec3{-0.5f}},
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
    auto const testCases = osc::to_array<osc::doc::Variant>(
    {
        osc::doc::Variant{false},
        osc::doc::Variant{true},
        osc::doc::Variant{osc::Color::white()},
        osc::doc::Variant{osc::Color::black()},
        osc::doc::Variant{osc::Color::clear()},
        osc::doc::Variant{osc::Color::magenta()},
        osc::doc::Variant{-1.0f},
        osc::doc::Variant{0.0f},
        osc::doc::Variant{-30.0f},
        osc::doc::Variant{std::numeric_limits<float>::quiet_NaN()},
        osc::doc::Variant{std::numeric_limits<float>::signaling_NaN()},
        osc::doc::Variant{std::numeric_limits<float>::infinity()},
        osc::doc::Variant{-std::numeric_limits<float>::infinity()},
        osc::doc::Variant{std::numeric_limits<int>::min()},
        osc::doc::Variant{std::numeric_limits<int>::max()},
        osc::doc::Variant{-1},
        osc::doc::Variant{0},
        osc::doc::Variant{1},
        osc::doc::Variant{""},
        osc::doc::Variant{"false"},
        osc::doc::Variant{"true"},
        osc::doc::Variant{"0"},
        osc::doc::Variant{"1"},
        osc::doc::Variant{"a string"},
        osc::doc::Variant{osc::StringName{"a string name"}},
        osc::doc::Variant{glm::vec3{}},
        osc::doc::Variant{glm::vec3{1.0f}},
        osc::doc::Variant{glm::vec3{-1.0f}},
        osc::doc::Variant{glm::vec3{0.5f}},
        osc::doc::Variant{glm::vec3{-0.5f}},
    });

    for (auto const& testCase : testCases)
    {
        ASSERT_NO_THROW({ std::hash<osc::doc::Variant>{}(testCase); });
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
        osc::doc::Variant const variant{s};
        auto const hash = std::hash<osc::doc::Variant>{}(variant);

        ASSERT_EQ(hash, std::hash<std::string>{}(std::string{s}));
        ASSERT_EQ(hash, std::hash<std::string_view>{}(s));
        ASSERT_EQ(hash, std::hash<osc::CStringView>{}(std::string{s}));
    }
}

TEST(Variant, ConstructingFromstringNameMakesGetTypeReturnStringNameType)
{
    ASSERT_EQ(osc::doc::Variant(osc::StringName{"s"}).getType(), osc::doc::VariantType::StringName);
}

TEST(Variant, ConstructedFromSameStringNameComparesEquivalent)
{
    ASSERT_EQ(osc::doc::Variant{osc::StringName{"string"}}, osc::doc::Variant{osc::StringName{"string"}});
}

TEST(Variant, ConstructedFromStringNameComparesInequivalentToVariantConstructedFromDifferentString)
{
    ASSERT_NE(osc::doc::Variant{osc::StringName{"a"}}, osc::doc::Variant{std::string{"b"}});
}

TEST(Variant, StringNameToBoolReturnsExpectedBoolValues)
{
    ASSERT_EQ(osc::doc::Variant(osc::StringName{"false"}).toBool(), false);
    ASSERT_EQ(osc::doc::Variant(osc::StringName{"FALSE"}).toBool(), false);
    ASSERT_EQ(osc::doc::Variant(osc::StringName{"False"}).toBool(), false);
    ASSERT_EQ(osc::doc::Variant(osc::StringName{"FaLsE"}).toBool(), false);
    ASSERT_EQ(osc::doc::Variant(osc::StringName{"0"}).toBool(), false);
    ASSERT_EQ(osc::doc::Variant(osc::StringName{""}).toBool(), false);

    // all other strings are effectively `true`
    ASSERT_EQ(osc::doc::Variant(osc::StringName{"true"}).toBool(), true);
    ASSERT_EQ(osc::doc::Variant(osc::StringName{"non-empty string"}).toBool(), true);
    ASSERT_EQ(osc::doc::Variant(osc::StringName{" "}).toBool(), true);
}

TEST(Variant, StringNameToColorWorksIfStringIsAValidHTMLColorString)
{
    ASSERT_EQ(osc::doc::Variant{osc::StringName{"#ff0000ff"}}.toColor(), osc::Color::red());
    ASSERT_EQ(osc::doc::Variant{osc::StringName{"#00ff00ff"}}.toColor(), osc::Color::green());
    ASSERT_EQ(osc::doc::Variant{osc::StringName{"#ffffffff"}}.toColor(), osc::Color::white());
    ASSERT_EQ(osc::doc::Variant{osc::StringName{"#00000000"}}.toColor(), osc::Color::clear());
    ASSERT_EQ(osc::doc::Variant{osc::StringName{"#000000ff"}}.toColor(), osc::Color::black());
    ASSERT_EQ(osc::doc::Variant{osc::StringName{"#000000FF"}}.toColor(), osc::Color::black());
    ASSERT_EQ(osc::doc::Variant{osc::StringName{"#123456ae"}}.toColor(), *osc::TryParseHtmlString("#123456ae"));
}

TEST(Variant, StringNameToColorReturnsBlackIfStringIsInvalidHTMLColorString)
{
    ASSERT_EQ(osc::doc::Variant{osc::StringName{"not a color"}}.toColor(), osc::Color::black());
}

TEST(Variant, StringNameToFloatTriesToParseStringAsFloatAndReturnsZeroOnFailure)
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
        ASSERT_EQ(osc::doc::Variant{osc::StringName{input}}.toFloat(), expectedOutput);
    }
}

TEST(Variant, StringNameToIntTriesToParseStringAsBase10Int)
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
        ASSERT_EQ(osc::doc::Variant{osc::StringName{input}}.toInt(), expectedOutput);
    }
}

TEST(Variant, StringNameToStringReturnsSuppliedString)
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
        ASSERT_EQ(osc::doc::Variant{osc::StringName{input}}.toString(), input);
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
        ASSERT_EQ(osc::doc::Variant{osc::StringName{input}}.toVec3(), glm::vec3{});
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
        auto snv = osc::doc::Variant{osc::StringName{input}};
        auto sv = osc::doc::Variant{std::string{input}};

        ASSERT_EQ(std::hash<osc::doc::Variant>{}(snv), std::hash<osc::doc::Variant>{}(sv));
    }
}
