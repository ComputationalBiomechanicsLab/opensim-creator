#include <oscar/DOM/PropertyInfo.h>

#include <oscar/Variant/Variant.h>
#include <oscar/Variant/VariantType.h>

#include <gtest/gtest.h>

using namespace osc;

TEST(PropertyInfo, has_no_name_and_nil_default_value_when_default_constructed)
{
    PropertyInfo info;
    ASSERT_EQ(info.name(), "");
    ASSERT_EQ(info.type(), VariantType::None);
    ASSERT_EQ(info.default_value(), Variant{});
}

TEST(PropertyInfo, can_construct_from_name_and_Variant_default_value)
{
    [[maybe_unused]] const PropertyInfo desc{"name", Variant{1.0f}};
}

TEST(PropertyInfo, name_returns_name_supplied_via_constructor)
{
    const PropertyInfo desc{"suppliedName", Variant{1.0f}};
    ASSERT_EQ(desc.name(), "suppliedName");
}

TEST(PropertyInfo, can_provide_a_std_string_prvalue_as_the_name_via_the_constructor)
{
    ASSERT_NO_THROW({ PropertyInfo(std::string{"rvalue"}, Variant{false}); });
}

TEST(PropertyInfo, type_returns_the_default_argument_Variant_type_supplied_via_the_constructor)
{
    const PropertyInfo desc{"name", Variant{1.0f}};
    ASSERT_EQ(desc.type(), VariantType::Float);
}

TEST(PropertyInfo, compares_equal_to_another_PropertyInfo_with_the_same_name_and_default_value)
{
    const PropertyInfo a{"name", Variant{1.0f}};
    const PropertyInfo b{"name", Variant{1.0f}};
    ASSERT_EQ(a, b);
}

TEST(PropertyInfo, compares_not_equal_to_another_PropertyInfo_with_a_different_name)
{
    const PropertyInfo a{"a", Variant{1.0f}};
    const PropertyInfo b{"b", Variant{1.0f}};
    ASSERT_NE(a, b);
}

TEST(PropertyInfo, compares_not_equal_to_another_PropertyInfo_with_same_name_but_different_default_value)
{
    const PropertyInfo a{"name", Variant{1.0f}};
    const PropertyInfo b{"name", Variant{2.0f}};
    ASSERT_NE(a, b);
}

TEST(PropertyInfo, compares_not_equal_to_another_PropertyInfo_with_same_name_but_different_default_value_type)
{
    const PropertyInfo a{"name", Variant{1.0f}};
    const PropertyInfo b{"name", Variant{"different type"}};
    ASSERT_NE(a, b);
}

TEST(PropertyInfo, constructor_throws_an_exception_if_name_contains_whitespace)
{
    // (basic examples)

    ASSERT_ANY_THROW({ PropertyInfo(" leadingSpace", Variant{true}); });
    ASSERT_ANY_THROW({ PropertyInfo("trailingSpace ", Variant{true}); });
    ASSERT_ANY_THROW({ PropertyInfo("inner space", Variant{true}); });

    ASSERT_ANY_THROW({ PropertyInfo("\nleadingNewline", Variant{true}); });
    ASSERT_ANY_THROW({ PropertyInfo("trailingNewline\n", Variant{true}); });
    ASSERT_ANY_THROW({ PropertyInfo("inner\newline", Variant{true}); });

    ASSERT_ANY_THROW({ PropertyInfo("\tleadingTab", Variant{true}); });
    ASSERT_ANY_THROW({ PropertyInfo("trailingTab\t", Variant{true}); });
    ASSERT_ANY_THROW({ PropertyInfo("inner\tTab", Variant{true}); });
}

TEST(PropertyInfo, constructor_throws_exception_if_name_contains_any_ASCII_control_characters)
{
    const auto test = [](char c)
    {
        ASSERT_ANY_THROW({ PropertyInfo(std::string{c} + std::string("leading"), Variant(true)); });
        ASSERT_ANY_THROW({ PropertyInfo(std::string("trailing") + c, Variant(true)); });
        ASSERT_ANY_THROW({ PropertyInfo(std::string("inner") + c + std::string("usage"), Variant{true}); });
    };

    constexpr char c_last_acii_control_character = 0x1F;
    for (char c = 0; c <= c_last_acii_control_character; ++c) {
        test(c);
    }
    test(0x7F);  // DEL
}
