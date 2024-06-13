#include <oscar/Platform/AppSettingValue.h>

#include <oscar/Graphics/Color.h>
#include <oscar/Utils/CStringView.h>

#include <gtest/gtest.h>

#include <array>
#include <string>

using namespace osc;

TEST(AppSettingValue, CanExplicitlyConstructFromStringRValue)
{
    AppSettingValue v{std::string{"stringrval"}};

    ASSERT_EQ(v.to_string(), "stringrval");
}

TEST(AppSettingValue, CanExplicitlyConstructFromStringLiteral)
{
    AppSettingValue v{"cstringliteral"};
    ASSERT_EQ(v.to_string(), "cstringliteral");
}

TEST(AppSettingValue, CanExplicitlyConstructFromCStringView)
{
    AppSettingValue v{CStringView{"cstringview"}};
    ASSERT_EQ(v.to_string(), "cstringview");
}

TEST(AppSettingValue, CanExplicitlyContructFromBool)
{
    AppSettingValue vfalse{false};
    ASSERT_EQ(vfalse.to_bool(), false);
    AppSettingValue vtrue{true};
    ASSERT_EQ(vtrue.to_bool(), true);
}

TEST(AppSettingValue, CanExplicitlyConstructFromColor)
{
    AppSettingValue v{Color::red()};
    ASSERT_EQ(v.to_color(), Color::red());
}

TEST(AppSettingValue, BoolValueToStringReturnsExpectedStrings)
{
    AppSettingValue vfalse{false};
    ASSERT_EQ(vfalse.to_string(), "false");
    AppSettingValue vtrue{true};
    ASSERT_EQ(vtrue.to_string(), "true");
}

TEST(AppSettingValue, StringValueToBoolReturnsExpectedBoolValues)
{
    ASSERT_EQ(AppSettingValue{"false"}.to_bool(), false);
    ASSERT_EQ(AppSettingValue{"FALSE"}.to_bool(), false);
    ASSERT_EQ(AppSettingValue{"False"}.to_bool(), false);
    ASSERT_EQ(AppSettingValue{"FaLsE"}.to_bool(), false);
    ASSERT_EQ(AppSettingValue{"0"}.to_bool(), false);
    ASSERT_EQ(AppSettingValue{""}.to_bool(), false);

    // all other strings are effectively `true`
    ASSERT_EQ(AppSettingValue{"true"}.to_bool(), true);
    ASSERT_EQ(AppSettingValue{"non-empty string"}.to_bool(), true);
    ASSERT_EQ(AppSettingValue{" "}.to_bool(), true);
}

TEST(AppSettingValue, ColorValueToStringReturnsSameAsToHtmlStringRGBA)
{
    const auto colors = std::to_array({
        Color::red(),
        Color::magenta(),
    });

    for (const auto& color : colors) {
        ASSERT_EQ(AppSettingValue{color}.to_string(), to_html_string_rgba(color));
    }
}

TEST(AppSettingValue, ColorValueToStringReturnsExpectedManualExamples)
{
    ASSERT_EQ(AppSettingValue{Color::yellow()}.to_string(), "#ffff00ff");
    ASSERT_EQ(AppSettingValue{Color::magenta()}.to_string(), "#ff00ffff");
}

TEST(AppSettingValue, StringValueToColorWorksIfStringIsAValidHTMLColorString)
{
    ASSERT_EQ(AppSettingValue{"#ff0000ff"}.to_color(), Color::red());
    ASSERT_EQ(AppSettingValue{"#00ff00ff"}.to_color(), Color::green());
    ASSERT_EQ(AppSettingValue{"#ffffffff"}.to_color(), Color::white());
    ASSERT_EQ(AppSettingValue{"#00000000"}.to_color(), Color::clear());
    ASSERT_EQ(AppSettingValue{"#000000ff"}.to_color(), Color::black());
    ASSERT_EQ(AppSettingValue{"#000000FF"}.to_color(), Color::black());
    ASSERT_EQ(AppSettingValue{"#123456ae"}.to_color(), *try_parse_html_color_string("#123456ae"));
}

TEST(AppSettingValue, StringValueColorReturnsWhiteIfStringIsInvalidHTMLColorString)
{
    ASSERT_EQ(AppSettingValue{"not a color"}.to_color(), Color::white());
}
