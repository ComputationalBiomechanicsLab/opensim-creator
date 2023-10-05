#include <oscar/Platform/AppSettingValue.hpp>

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
