#include <oscar/Platform/AppSettingValue.hpp>

#include <oscar/Graphics/Color.hpp>
#include <oscar/Utils/CStringView.hpp>

#include <gtest/gtest.h>

#include <string>

TEST(AppSettingValue, CanExplicitlyConstructFromStringRValue)
{
    osc::AppSettingValue v{std::string{"stringrval"}};

    ASSERT_EQ(v.toString(), "stringrval");
}

TEST(AppSettingValue, CanExplicitlyConstructFromStringLiteral)
{
    osc::AppSettingValue v{"cstringliteral"};
    ASSERT_EQ(v.toString(), "cstringliteral");
}

TEST(AppSettingValue, CanExplicitlyConstructFromCStringView)
{
    osc::AppSettingValue v{osc::CStringView{"cstringview"}};
    ASSERT_EQ(v.toString(), "cstringview");
}

TEST(AppSettingValue, CanExplicitlyContructFromBool)
{
    osc::AppSettingValue vfalse{false};
    ASSERT_EQ(vfalse.toBool(), false);
    osc::AppSettingValue vtrue{true};
    ASSERT_EQ(vtrue.toBool(), true);
}

TEST(AppSettingValue, CanExplicitlyConstructFromColor)
{
    osc::AppSettingValue v{osc::Color::red()};
    ASSERT_EQ(v.toColor(), osc::Color::red());
}

TEST(AppSettingValue, BoolValueToStringReturnsExpectedStrings)
{
    osc::AppSettingValue vfalse{false};
    ASSERT_EQ(vfalse.toString(), "false");
    osc::AppSettingValue vtrue{true};
    ASSERT_EQ(vtrue.toString(), "true");
}

TEST(AppSettingValue, StringValueToBoolReturnsExpectedBoolValues)
{
    ASSERT_EQ(osc::AppSettingValue{"false"}.toBool(), false);
    ASSERT_EQ(osc::AppSettingValue{"FALSE"}.toBool(), false);
    ASSERT_EQ(osc::AppSettingValue{"False"}.toBool(), false);
    ASSERT_EQ(osc::AppSettingValue{"FaLsE"}.toBool(), false);
    ASSERT_EQ(osc::AppSettingValue{"0"}.toBool(), false);
    ASSERT_EQ(osc::AppSettingValue{""}.toBool(), false);

    // all other strings are effectively `true`
    ASSERT_EQ(osc::AppSettingValue{"true"}.toBool(), true);
    ASSERT_EQ(osc::AppSettingValue{"non-empty string"}.toBool(), true);
    ASSERT_EQ(osc::AppSettingValue{" "}.toBool(), true);
}

TEST(AppSettingValue, ColorValueToStringReturnsSameAsToHtmlStringRGBA)
{
    auto const colors = osc::to_array(
    {
        osc::Color::red(),
        osc::Color::magenta(),
    });

    for (auto const& color : colors)
    {
        ASSERT_EQ(osc::AppSettingValue{color}.toString(), osc::ToHtmlStringRGBA(color));
    }
}

TEST(AppSettingValue, ColorValueToStringReturnsExpectedManualExamples)
{
    ASSERT_EQ(osc::AppSettingValue{osc::Color::yellow()}.toString(), "#ffff00ff");
    ASSERT_EQ(osc::AppSettingValue{osc::Color::magenta()}.toString(), "#ff00ffff");
}

TEST(AppSettingValue, StringValueToColorWorksIfStringIsAValidHTMLColorString)
{
    ASSERT_EQ(osc::AppSettingValue{"#ff0000ff"}.toColor(), osc::Color::red());
    ASSERT_EQ(osc::AppSettingValue{"#00ff00ff"}.toColor(), osc::Color::green());
    ASSERT_EQ(osc::AppSettingValue{"#ffffffff"}.toColor(), osc::Color::white());
    ASSERT_EQ(osc::AppSettingValue{"#00000000"}.toColor(), osc::Color::clear());
    ASSERT_EQ(osc::AppSettingValue{"#000000ff"}.toColor(), osc::Color::black());
    ASSERT_EQ(osc::AppSettingValue{"#000000FF"}.toColor(), osc::Color::black());
    ASSERT_EQ(osc::AppSettingValue{"#123456ae"}.toColor(), *osc::TryParseHtmlString("#123456ae"));
}

TEST(AppSettingValue, StringValueColorReturnsWhiteIfStringIsInvalidHTMLColorString)
{
    ASSERT_EQ(osc::AppSettingValue{"not a color"}.toColor(), osc::Color::white());
}
