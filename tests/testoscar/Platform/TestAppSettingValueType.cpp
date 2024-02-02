#include <oscar/Platform/AppSettingValue.hpp>

#include <oscar/Graphics/Color.hpp>
#include <oscar/Utils/CStringView.hpp>

#include <gtest/gtest.h>

#include <array>
#include <string>

using osc::AppSettingValue;
using osc::Color;
using osc::CStringView;
using osc::TryParseHtmlString;

TEST(AppSettingValue, CanExplicitlyConstructFromStringRValue)
{
    AppSettingValue v{std::string{"stringrval"}};

    ASSERT_EQ(v.toString(), "stringrval");
}

TEST(AppSettingValue, CanExplicitlyConstructFromStringLiteral)
{
    AppSettingValue v{"cstringliteral"};
    ASSERT_EQ(v.toString(), "cstringliteral");
}

TEST(AppSettingValue, CanExplicitlyConstructFromCStringView)
{
    AppSettingValue v{CStringView{"cstringview"}};
    ASSERT_EQ(v.toString(), "cstringview");
}

TEST(AppSettingValue, CanExplicitlyContructFromBool)
{
    AppSettingValue vfalse{false};
    ASSERT_EQ(vfalse.toBool(), false);
    AppSettingValue vtrue{true};
    ASSERT_EQ(vtrue.toBool(), true);
}

TEST(AppSettingValue, CanExplicitlyConstructFromColor)
{
    AppSettingValue v{Color::red()};
    ASSERT_EQ(v.toColor(), Color::red());
}

TEST(AppSettingValue, BoolValueToStringReturnsExpectedStrings)
{
    AppSettingValue vfalse{false};
    ASSERT_EQ(vfalse.toString(), "false");
    AppSettingValue vtrue{true};
    ASSERT_EQ(vtrue.toString(), "true");
}

TEST(AppSettingValue, StringValueToBoolReturnsExpectedBoolValues)
{
    ASSERT_EQ(AppSettingValue{"false"}.toBool(), false);
    ASSERT_EQ(AppSettingValue{"FALSE"}.toBool(), false);
    ASSERT_EQ(AppSettingValue{"False"}.toBool(), false);
    ASSERT_EQ(AppSettingValue{"FaLsE"}.toBool(), false);
    ASSERT_EQ(AppSettingValue{"0"}.toBool(), false);
    ASSERT_EQ(AppSettingValue{""}.toBool(), false);

    // all other strings are effectively `true`
    ASSERT_EQ(AppSettingValue{"true"}.toBool(), true);
    ASSERT_EQ(AppSettingValue{"non-empty string"}.toBool(), true);
    ASSERT_EQ(AppSettingValue{" "}.toBool(), true);
}

TEST(AppSettingValue, ColorValueToStringReturnsSameAsToHtmlStringRGBA)
{
    auto const colors = std::to_array(
    {
        Color::red(),
        Color::magenta(),
    });

    for (auto const& color : colors)
    {
        ASSERT_EQ(AppSettingValue{color}.toString(), ToHtmlStringRGBA(color));
    }
}

TEST(AppSettingValue, ColorValueToStringReturnsExpectedManualExamples)
{
    ASSERT_EQ(AppSettingValue{Color::yellow()}.toString(), "#ffff00ff");
    ASSERT_EQ(AppSettingValue{Color::magenta()}.toString(), "#ff00ffff");
}

TEST(AppSettingValue, StringValueToColorWorksIfStringIsAValidHTMLColorString)
{
    ASSERT_EQ(AppSettingValue{"#ff0000ff"}.toColor(), Color::red());
    ASSERT_EQ(AppSettingValue{"#00ff00ff"}.toColor(), Color::green());
    ASSERT_EQ(AppSettingValue{"#ffffffff"}.toColor(), Color::white());
    ASSERT_EQ(AppSettingValue{"#00000000"}.toColor(), Color::clear());
    ASSERT_EQ(AppSettingValue{"#000000ff"}.toColor(), Color::black());
    ASSERT_EQ(AppSettingValue{"#000000FF"}.toColor(), Color::black());
    ASSERT_EQ(AppSettingValue{"#123456ae"}.toColor(), *TryParseHtmlString("#123456ae"));
}

TEST(AppSettingValue, StringValueColorReturnsWhiteIfStringIsInvalidHTMLColorString)
{
    ASSERT_EQ(AppSettingValue{"not a color"}.toColor(), Color::white());
}
