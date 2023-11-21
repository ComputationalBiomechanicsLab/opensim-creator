#include <oscar/Utils/Variant.hpp>

#include <gtest/gtest.h>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/StringName.hpp>

#include <array>
#include <charconv>
#include <stdexcept>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

using osc::Color;
using osc::StringName;
using osc::Variant;
using osc::VariantType;
using osc::Vec3;

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
    ASSERT_EQ(vfalse.to<bool>(), false);
    Variant vtrue{true};
    ASSERT_EQ(vtrue.to<bool>(), true);

    ASSERT_EQ(vtrue.getType(), VariantType::Bool);
}

TEST(Variant, CanImplicitlyConstructFromBool)
{
    static_assert(std::is_convertible_v<bool, Variant>);
}

TEST(Variant, CanExplicitlyConstructFromColor)
{
    Variant v{Color::red()};
    ASSERT_EQ(v.to<Color>(), Color::red());
    ASSERT_EQ(v.getType(), VariantType::Color);
}

TEST(Variant, CanImplicitlyConstructFromColor)
{
    static_assert(std::is_convertible_v<Color, Variant>);
}

TEST(Variant, CanExplicityConstructFromFloat)
{
    Variant v{1.0f};
    ASSERT_EQ(v.to<float>(), 1.0f);
    ASSERT_EQ(v.getType(), VariantType::Float);
}

TEST(Variant, CanImplicitlyConstructFromFloat)
{
    static_assert(std::is_convertible_v<float, Variant>);
}

TEST(Variant, CanExplicitlyConstructFromInt)
{
    Variant v{5};
    ASSERT_EQ(v.to<int>(), 5);
    ASSERT_EQ(v.getType(), VariantType::Int);
}

TEST(Variant, CanImplicitlyConstructFromInt)
{
    static_assert(std::is_convertible_v<int, Variant>);
}

TEST(Variant, CanExplicitlyConstructFromStringRValue)
{
    Variant v{std::string{"stringrval"}};
    ASSERT_EQ(v.to<std::string>(), "stringrval");
    ASSERT_EQ(v.getType(), VariantType::String);
}

TEST(Variant, CanImplicitlyConstructFromStringRValue)
{
    static_assert(std::is_convertible_v<std::string&&, Variant>);
}

TEST(Variant, CanExplicitlyConstructFromStringLiteral)
{
    Variant v{"cstringliteral"};
    ASSERT_EQ(v.to<std::string>(), "cstringliteral");
    ASSERT_EQ(v.getType(), VariantType::String);
}

TEST(Variant, CanImplicitlyConstructFromStringLiteral)
{
    static_assert(std::is_convertible_v<decltype(""), Variant>);
}

TEST(Variant, CanExplicitlyConstructFromCStringView)
{
    Variant v{osc::CStringView{"cstringview"}};
    ASSERT_EQ(v.to<std::string>(), "cstringview");
    ASSERT_EQ(v.getType(), VariantType::String);
}

TEST(Variant, CanImplicitlyConstructFromCStringView)
{
    static_assert(std::is_convertible_v<osc::CStringView, Variant>);
}

TEST(Variant, CanExplicitlyConstructFromVec3)
{
    Variant v{Vec3{1.0f, 2.0f, 3.0f}};
    ASSERT_EQ(v.to<Vec3>(), Vec3(1.0f, 2.0f, 3.0f));
    ASSERT_EQ(v.getType(), VariantType::Vec3);
}

TEST(Variant, CanImplicitlyConstructFromVec3)
{
    static_assert(std::is_convertible_v<Vec3, Variant>);
}

TEST(Variant, DefaultConstructedValueIsNil)
{
    ASSERT_EQ(Variant{}.getType(), VariantType::Nil);
}

TEST(Variant, NilValueToBoolReturnsFalse)
{
    ASSERT_EQ(Variant{}.to<bool>(), false);
}

TEST(Variant, NilValueToColorReturnsBlack)
{
    ASSERT_EQ(Variant{}.to<Color>(), Color::black());
}

TEST(Variant, NilValueToFloatReturnsZero)
{
    ASSERT_EQ(Variant{}.to<float>(), 0.0f);
}

TEST(Variant, NilValueToIntReturnsZero)
{
    ASSERT_EQ(Variant{}.to<int>(), 0);
}

TEST(Variant, NilValueToStringReturnsNull)
{
    ASSERT_EQ(Variant{}.to<std::string>(), "<null>");
}

TEST(Variant, NilValueToStringNameReturnsEmptyStringName)
{
    ASSERT_EQ(Variant{}.to<StringName>(), StringName{});
}

TEST(Variant, NilValueToVec3ReturnsZeroedVec3)
{
    ASSERT_EQ(Variant{}.to<Vec3>(), Vec3{});
}

TEST(Variant, BoolValueToBoolReturnsExpectedBools)
{
    ASSERT_EQ(Variant(false).to<bool>(), false);
    ASSERT_EQ(Variant(true).to<bool>(), true);
}

TEST(Variant, BoolValueToColorReturnsExpectedColors)
{
    ASSERT_EQ(Variant(false).to<Color>(), Color::black());
    ASSERT_EQ(Variant(true).to<Color>(), Color::white());
}

TEST(Variant, BoolValueToFloatReturnsExpectedFloats)
{
    ASSERT_EQ(Variant(false).to<float>(), 0.0f);
    ASSERT_EQ(Variant(true).to<float>(), 1.0f);
}

TEST(Variant, BoolValueToIntReturnsExpectedInts)
{
    ASSERT_EQ(Variant(false).to<int>(), 0);
    ASSERT_EQ(Variant(true).to<int>(), 1);
}

TEST(Variant, BoolValueToStringReturnsExpectedStrings)
{
    Variant vfalse{false};
    ASSERT_EQ(vfalse.to<std::string>(), "false");
    Variant vtrue{true};
    ASSERT_EQ(vtrue.to<std::string>(), "true");
}

TEST(Variant, BoolValueToStringNameReturnsEmptyStringName)
{
    ASSERT_EQ(Variant{false}.to<StringName>(), StringName{});
    ASSERT_EQ(Variant{true}.to<StringName>(), StringName{});
}

TEST(Variant, BoolValueToVec3ReturnsExpectedVectors)
{
    ASSERT_EQ(Variant(false).to<Vec3>(), Vec3{});
    ASSERT_EQ(Variant(true).to<Vec3>(), Vec3(1.0f, 1.0f, 1.0f));
}

TEST(Variant, ColorToBoolReturnsExpectedValues)
{
    ASSERT_EQ(Variant(Color::black()).to<bool>(), false);
    ASSERT_EQ(Variant(Color::white()).to<bool>(), true);
    ASSERT_EQ(Variant(Color::magenta()).to<bool>(), true);
}

TEST(Variant, ColorToColorReturnsExpectedValues)
{
    ASSERT_EQ(Variant(Color::black()).to<Color>(), Color::black());
    ASSERT_EQ(Variant(Color::red()).to<Color>(), Color::red());
    ASSERT_EQ(Variant(Color::yellow()).to<Color>(), Color::yellow());
}

TEST(Variant, ColorToFloatReturnsExpectedValues)
{
    // should only extract first channel, to match vec3 behavior for conversion
    ASSERT_EQ(Variant(Color::black()).to<float>(), 0.0f);
    ASSERT_EQ(Variant(Color::white()).to<float>(), 1.0f);
    ASSERT_EQ(Variant(Color::blue()).to<float>(), 0.0f);
}

TEST(Variant, ColorToIntReturnsExpectedValues)
{
    // should only extract first channel, to match vec3 behavior for conversion
    ASSERT_EQ(Variant(Color::black()).to<int>(), 0);
    ASSERT_EQ(Variant(Color::white()).to<int>(), 1);
    ASSERT_EQ(Variant(Color::cyan()).to<int>(), 0);
    ASSERT_EQ(Variant(Color::yellow()).to<int>(), 1);
}

TEST(Variant, ColorValueToStringReturnsSameAsToHtmlStringRGBA)
{
    auto const colors = std::to_array(
    {
        Color::red(),
        Color::magenta(),
    });

    for (auto const& color : colors)
    {
        ASSERT_EQ(Variant(color).to<std::string>(), osc::ToHtmlStringRGBA(color));
    }
}

TEST(Variant, ColorValueToStringReturnsExpectedManualExamples)
{
    ASSERT_EQ(Variant{Color::yellow()}.to<std::string>(), "#ffff00ff");
    ASSERT_EQ(Variant{Color::magenta()}.to<std::string>(), "#ff00ffff");
}

TEST(Variant, ColorValueToVec3ReturnsFirst3Channels)
{
    ASSERT_EQ(Variant(Color(1.0f, 2.0f, 3.0f)).to<Vec3>(), Vec3(1.0f, 2.0f, 3.0f));
    ASSERT_EQ(Variant(Color::red()).to<Vec3>(), Vec3(1.0f, 0.0f, 0.0f));
}

TEST(Variant, FloatValueToBoolReturnsExpectedValues)
{
    ASSERT_EQ(Variant(0.0f).to<bool>(), false);
    ASSERT_EQ(Variant(-0.5f).to<bool>(), true);
    ASSERT_EQ(Variant(-1.0f).to<bool>(), true);
    ASSERT_EQ(Variant(1.0f).to<bool>(), true);
    ASSERT_EQ(Variant(0.75f).to<bool>(), true);
}

TEST(Variant, FloatValueToColorReturnsExpectedColor)
{
    for (float v : std::to_array<float>({0.0f, 0.5f, 0.75, 1.0f}))
    {
        Color const expected = {v, v, v};
        ASSERT_EQ(Variant(v).to<Color>(), expected);
    }
}

TEST(Variant, FloatValueToFloatReturnsInput)
{
    ASSERT_EQ(Variant(0.0f).to<float>(), 0.0f);
    ASSERT_EQ(Variant(0.12345f).to<float>(), 0.12345f);
    ASSERT_EQ(Variant(-0.54321f).to<float>(), -0.54321f);
}

TEST(Variant, FloatValueToIntReturnsCastedResult)
{
    for (float v : std::to_array<float>({-0.5f, -0.123f, 0.0f, 1.0f, 1337.0f}))
    {
        int const expected = static_cast<int>(v);
        ASSERT_EQ(Variant(v).to<int>(), expected);
    }
}

TEST(Variant, FloatValueToStringReturnsToStringedResult)
{
    for (float v : std::to_array<float>({-5.35f, -2.0f, -1.0f, 0.0f, 0.123f, 18000.0f}))
    {
        std::string const expected = std::to_string(v);
        ASSERT_EQ(Variant(v).to<std::string>(), expected);
    }
}

TEST(Variant, FloatValueToStringNameReturnsEmptyStringName)
{
    ASSERT_EQ(Variant{0.0f}.to<StringName>(), StringName{});
    ASSERT_EQ(Variant{1.0f}.to<StringName>(), StringName{});
}

TEST(Variant, FloatValueToVec3ReturnsVec3FilledWithFloat)
{
    for (float v : std::to_array<float>({-20000.0f, -5.328f, -1.2f, 0.0f, 0.123f, 50.0f, 18000.0f}))
    {
        Vec3 const expected = {v, v, v};
        ASSERT_EQ(Variant(v).to<Vec3>(), expected);
    }
}

TEST(Variant, IntValueToBoolReturnsExpectedResults)
{
    ASSERT_EQ(Variant(0).to<bool>(), false);
    ASSERT_EQ(Variant(1).to<bool>(), true);
    ASSERT_EQ(Variant(-1).to<bool>(), true);
    ASSERT_EQ(Variant(234056).to<bool>(), true);
    ASSERT_EQ(Variant(-12938).to<bool>(), true);
}

TEST(Variant, IntValueToColorReturnsBlackOrWhite)
{
    ASSERT_EQ(Variant(0).to<Color>(), Color::black());
    ASSERT_EQ(Variant(1).to<Color>(), Color::white());
    ASSERT_EQ(Variant(-1).to<Color>(), Color::white());
    ASSERT_EQ(Variant(-230244).to<Color>(), Color::white());
    ASSERT_EQ(Variant(100983).to<Color>(), Color::white());
}

TEST(Variant, IntValueToFloatReturnsIntCastedToFloat)
{
    for (int v : std::to_array<int>({-10000, -1000, -1, 0, 1, 17, 23000}))
    {
        auto const expected = static_cast<float>(v);
        ASSERT_EQ(Variant(v).to<float>(), expected);
    }
}

TEST(Variant, IntValueToIntReturnsTheSuppliedInt)
{
    for (int v : std::to_array<int>({ -123028, -2381, -32, -2, 0, 1, 1488, 5098}))
    {
        ASSERT_EQ(Variant(v).to<int>(), v);
    }
}

TEST(Variant, IntValueToStringReturnsStringifiedInt)
{
    for (int v : std::to_array<int>({ -121010, -13482, -1923, -123, -92, -7, 0, 1, 1294, 1209849}))
    {
        std::string const expected = std::to_string(v);
        ASSERT_EQ(Variant(v).to<std::string>(), expected);
    }
}

TEST(Variant, IntValueToStringNameReturnsEmptyString)
{
    ASSERT_EQ(Variant{-1}.to<StringName>(), StringName{});
    ASSERT_EQ(Variant{0}.to<StringName>(), StringName{});
    ASSERT_EQ(Variant{1337}.to<StringName>(), StringName{});
}

TEST(Variant, IntValueToVec3CastsValueToFloatThenPlacesInAllSlots)
{
    for (int v : std::to_array<int>({ -12193, -1212, -738, -12, -1, 0, 1, 18, 1294, 1209849}))
    {
        auto const vf = static_cast<float>(v);
        Vec3 const expected = {vf, vf, vf};
        ASSERT_EQ(Variant(v).to<Vec3>(), expected);
    }
}

TEST(Variant, StringValueToBoolReturnsExpectedBoolValues)
{
    ASSERT_EQ(Variant("false").to<bool>(), false);
    ASSERT_EQ(Variant("FALSE").to<bool>(), false);
    ASSERT_EQ(Variant("False").to<bool>(), false);
    ASSERT_EQ(Variant("FaLsE").to<bool>(), false);
    ASSERT_EQ(Variant("0").to<bool>(), false);
    ASSERT_EQ(Variant("").to<bool>(), false);

    // all other strings are effectively `true`
    ASSERT_EQ(Variant("true").to<bool>(), true);
    ASSERT_EQ(Variant("non-empty string").to<bool>(), true);
    ASSERT_EQ(Variant(" ").to<bool>(), true);
}

TEST(Variant, StringValueToColorWorksIfStringIsAValidHTMLColorString)
{
    ASSERT_EQ(Variant{"#ff0000ff"}.to<Color>(), Color::red());
    ASSERT_EQ(Variant{"#00ff00ff"}.to<Color>(), Color::green());
    ASSERT_EQ(Variant{"#ffffffff"}.to<Color>(), Color::white());
    ASSERT_EQ(Variant{"#00000000"}.to<Color>(), Color::clear());
    ASSERT_EQ(Variant{"#000000ff"}.to<Color>(), Color::black());
    ASSERT_EQ(Variant{"#000000FF"}.to<Color>(), Color::black());
    ASSERT_EQ(Variant{"#123456ae"}.to<Color>(), *osc::TryParseHtmlString("#123456ae"));
}

TEST(Variant, StringValueColorReturnsBlackIfStringIsInvalidHTMLColorString)
{
    ASSERT_EQ(Variant{"not a color"}.to<Color>(), Color::black());
}

TEST(Variant, StringValueToFloatTriesToParseStringAsFloatAndReturnsZeroOnFailure)
{
    auto const inputs = std::to_array<std::string_view>(
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
        ASSERT_EQ(Variant{input}.to<float>(), expectedOutput);
    }
}

TEST(Variant, StringValueToIntTriesToParseStringAsBase10Int)
{
    auto const inputs = std::to_array<std::string_view>(
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
        ASSERT_EQ(Variant{input}.to<int>(), expectedOutput);
    }
}

TEST(Variant, StringValueToStringReturnsSuppliedString)
{
    auto const inputs = std::to_array<std::string_view>(
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
        ASSERT_EQ(Variant{input}.to<std::string>(), input);
    }
}

TEST(Variant, StringValueToStringNameReturnsSuppliedStringAsAStringName)
{
    auto const inputs = std::to_array<std::string_view>(
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
        ASSERT_EQ(Variant{input}.to<StringName>(), StringName{input});
    }
}

TEST(Variant, StringValueToVec3AlwaysReturnsZeroedVec)
{
    auto const inputs = std::to_array<std::string_view>(
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
        ASSERT_EQ(Variant{input}.to<Vec3>(), Vec3{});
    }
}

TEST(Variant, Vec3ValueToBoolReturnsFalseForZeroVec)
{
    ASSERT_EQ(Variant{Vec3{}}.to<bool>(), false);
}

TEST(Variant, Vec3ValueToBoolReturnsFalseIfXIsZeroRegardlessOfOtherComponents)
{
    // why: because it's consistent with the `toInt()` and `toFloat()` behavior, and
    // one would logically expect `if (v.to<int>())` to behave the same as `if (v.to<bool>())`
    ASSERT_EQ(Variant{Vec3{0.0f}}.to<bool>(), false);
    ASSERT_EQ(Variant{Vec3(0.0f, 0.0f, 1000.0f)}.to<bool>(), false);
    ASSERT_EQ(Variant{Vec3(0.0f, 7.0f, -30.0f)}.to<bool>(), false);
    ASSERT_EQ(Variant{Vec3(0.0f, 2.0f, 1.0f)}.to<bool>(), false);
    ASSERT_EQ(Variant{Vec3(0.0f, 1.0f, 1.0f)}.to<bool>(), false);
    ASSERT_EQ(Variant{Vec3(0.0f, -1.0f, 0.0f)}.to<bool>(), false);
    static_assert(+0.0f == -0.0f);
    ASSERT_EQ(Variant{Vec3(-0.0f, 0.0f, 1000.0f)}.to<bool>(), false);  // how fun ;)
}

TEST(Variant, Vec3ValueToBoolReturnsTrueIfXIsNonZeroRegardlessOfOtherComponents)
{
    ASSERT_EQ(Variant{Vec3{1.0f}}.to<bool>(), true);
    ASSERT_EQ(Variant{Vec3(2.0f, 7.0f, -30.0f)}.to<bool>(), true);
    ASSERT_EQ(Variant{Vec3(30.0f, 2.0f, 1.0f)}.to<bool>(), true);
    ASSERT_EQ(Variant{Vec3(-40.0f, 1.0f, 1.0f)}.to<bool>(), true);
    ASSERT_EQ(Variant{Vec3(std::numeric_limits<float>::quiet_NaN(), -1.0f, 0.0f)}.to<bool>(), true);
}

TEST(Variant, Vec3ValueToColorExtractsTheElementsIntoRGB)
{
    auto const testCases = std::to_array<Vec3>(
    {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 1.0f},
        {-1.0f, 0.0f, 1.0f},
        {10.0f, 0.0f, 7.5f},
        {0.0f, -20.0f, 0.5f},
    });

    for (auto const& testCase : testCases)
    {
        ASSERT_EQ(Variant{testCase}.to<Color>(), Color{testCase});
    }
}

TEST(Variant, Vec3ValueToFloatExtractsXToTheFloat)
{
    auto const testCases = std::to_array<Vec3>(
    {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 1.0f},
        {-1.0f, 0.0f, 1.0f},
        {10.0f, 0.0f, 7.5f},
        {0.0f, -20.0f, 0.5f},
    });

    for (auto const& testCase : testCases)
    {
        ASSERT_EQ(Variant{testCase}.to<float>(), testCase.x);
    }
}

TEST(Variant, Vec3ValueToIntExtractsXToTheInt)
{
    auto const testCases = std::to_array<Vec3>(
    {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 1.0f},
        {-1.0f, 0.0f, 1.0f},
        {10.0f, 0.0f, 7.5f},
        {0.0f, -20.0f, 0.5f},
    });

    for (auto const& testCase : testCases)
    {
        ASSERT_EQ(Variant{testCase}.to<int>(), static_cast<int>(testCase.x));
    }
}

TEST(Variant, Vec3ValueToStringReturnsSameAsDirectlyConvertingVectorToString)
{
    auto const testCases = std::to_array<Vec3>(
    {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 1.0f},
        {-1.0f, 0.0f, 1.0f},
        {10.0f, 0.0f, 7.5f},
        {0.0f, -20.0f, 0.5f},
    });

    for (auto const& testCase : testCases)
    {
        ASSERT_EQ(Variant{testCase}.to<std::string>(), osc::to_string(testCase));
    }
}

TEST(Variant, Vec3ValueToStringNameReturnsAnEmptyString)
{
    ASSERT_EQ(Variant{Vec3{}}.to<StringName>(), StringName{});
    ASSERT_EQ(Variant{Vec3(0.0f, -20.0f, 0.5f)}.to<StringName>(), StringName{});
}

TEST(Variant, Vec3ValueToVec3ReturnsOriginalValue)
{
    auto const testCases = std::to_array<Vec3>(
    {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 1.0f},
        {-1.0f, 0.0f, 1.0f},
        {10.0f, 0.0f, 7.5f},
        {0.0f, -20.0f, 0.5f},
    });

    for (auto const& testCase : testCases)
    {
        ASSERT_EQ(Variant{testCase}.to<Vec3>(), testCase);
    }
}

TEST(Variant, IsAlwaysEqualToACopyOfItself)
{
    auto const testCases = std::to_array<Variant>(
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
        Variant{Vec3{}},
        Variant{Vec3{1.0f}},
        Variant{Vec3{-1.0f}},
        Variant{Vec3{0.5f}},
        Variant{Vec3{-0.5f}},
    });

    for (auto const& tc : testCases)
    {
        ASSERT_EQ(tc, tc) << "input: " << tc.to<std::string>();
    }

    auto const exceptions = std::to_array<Variant>(
    {
        Variant{std::numeric_limits<float>::quiet_NaN()},
        Variant{std::numeric_limits<float>::signaling_NaN()},
    });
    for (auto const& tc : exceptions)
    {
        ASSERT_NE(tc, tc) << "input: " << tc.to<std::string>();
    }
}

TEST(Variant, IsNotEqualToOtherValuesEvenIfConversionIsPossible)
{
    auto const testCases = std::to_array<Variant>(
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
    auto const testCases = std::to_array<Variant>(
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
        Variant{Vec3{}},
        Variant{Vec3{1.0f}},
        Variant{Vec3{-1.0f}},
        Variant{Vec3{0.5f}},
        Variant{Vec3{-0.5f}},
    });

    for (auto const& testCase : testCases)
    {
        ASSERT_NO_THROW({ std::hash<Variant>{}(testCase); });
    }
}

TEST(Variant, FreeFunctionToStringOnVarietyOfTypesReturnsSameAsCallingToStringMemberFunction)
{
    auto const testCases = std::to_array<Variant>(
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
        Variant{Vec3{}},
        Variant{Vec3{1.0f}},
        Variant{Vec3{-1.0f}},
        Variant{Vec3{0.5f}},
        Variant{Vec3{-0.5f}},
    });

    for (auto const& testCase : testCases)
    {
        ASSERT_EQ(to_string(testCase), testCase.to<std::string>());
    }
}

TEST(Variant, StreamingToOutputStreamProducesSameOutputAsToString)
{
    auto const testCases = std::to_array<Variant>(
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
        Variant{Vec3{}},
        Variant{Vec3{1.0f}},
        Variant{Vec3{-1.0f}},
        Variant{Vec3{0.5f}},
        Variant{Vec3{-0.5f}},
    });

    for (auto const& testCase : testCases)
    {
        std::stringstream ss;
        ss << testCase;
        ASSERT_EQ(ss.str(), testCase.to<std::string>());
    }
}

TEST(Variant, HashesForStringValuesMatchStdStringEtc)
{
    auto const strings = std::to_array<std::string_view>(
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
    ASSERT_EQ(Variant(osc::StringName{"false"}).to<bool>(), false);
    ASSERT_EQ(Variant(osc::StringName{"FALSE"}).to<bool>(), false);
    ASSERT_EQ(Variant(osc::StringName{"False"}).to<bool>(), false);
    ASSERT_EQ(Variant(osc::StringName{"FaLsE"}).to<bool>(), false);
    ASSERT_EQ(Variant(osc::StringName{"0"}).to<bool>(), false);
    ASSERT_EQ(Variant(osc::StringName{""}).to<bool>(), false);

    // all other strings are effectively `true`
    ASSERT_EQ(Variant(osc::StringName{"true"}).to<bool>(), true);
    ASSERT_EQ(Variant(osc::StringName{"non-empty string"}).to<bool>(), true);
    ASSERT_EQ(Variant(osc::StringName{" "}).to<bool>(), true);
}

TEST(Variant, StringNameValueToColorWorksIfStringIsAValidHTMLColorString)
{
    ASSERT_EQ(Variant{osc::StringName{"#ff0000ff"}}.to<Color>(), Color::red());
    ASSERT_EQ(Variant{osc::StringName{"#00ff00ff"}}.to<Color>(), Color::green());
    ASSERT_EQ(Variant{osc::StringName{"#ffffffff"}}.to<Color>(), Color::white());
    ASSERT_EQ(Variant{osc::StringName{"#00000000"}}.to<Color>(), Color::clear());
    ASSERT_EQ(Variant{osc::StringName{"#000000ff"}}.to<Color>(), Color::black());
    ASSERT_EQ(Variant{osc::StringName{"#000000FF"}}.to<Color>(), Color::black());
    ASSERT_EQ(Variant{osc::StringName{"#123456ae"}}.to<Color>(), *osc::TryParseHtmlString("#123456ae"));
}

TEST(Variant, StringNameValueToColorReturnsBlackIfStringIsInvalidHTMLColorString)
{
    ASSERT_EQ(Variant{osc::StringName{"not a color"}}.to<Color>(), Color::black());
}

TEST(Variant, StringNameValueToFloatTriesToParseStringAsFloatAndReturnsZeroOnFailure)
{
    auto const inputs = std::to_array<std::string_view>(
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
        ASSERT_EQ(Variant{osc::StringName{input}}.to<float>(), expectedOutput);
    }
}

TEST(Variant, StringNameValueToIntTriesToParseStringAsBase10Int)
{
    auto const inputs = std::to_array<std::string_view>(
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
        ASSERT_EQ(Variant{osc::StringName{input}}.to<int>(), expectedOutput);
    }
}

TEST(Variant, StringNameValueToStringReturnsSuppliedString)
{
    auto const inputs = std::to_array<std::string_view>(
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
        ASSERT_EQ(Variant{osc::StringName{input}}.to<std::string>(), input);
    }
}

TEST(Variant, StringNameValueToStringNameReturnsSuppliedStringName)
{
    auto const inputs = std::to_array<std::string_view>(
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
        ASSERT_EQ(Variant{StringName{input}}.to<StringName>(), StringName{input});
    }
}

TEST(Variant, StringNameToVec3AlwaysReturnsZeroedVec)
{
    auto const inputs = std::to_array<std::string_view>(
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
        ASSERT_EQ(Variant{osc::StringName{input}}.to<Vec3>(), Vec3{});
    }
}

TEST(Variant, HashOfStringNameVariantIsSameAsHashOfStringVariant)
{
    auto const inputs = std::to_array<std::string_view>(
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
    auto const inputs = std::to_array<std::string_view>(
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
    auto const inputs = std::to_array<std::string_view>(
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
