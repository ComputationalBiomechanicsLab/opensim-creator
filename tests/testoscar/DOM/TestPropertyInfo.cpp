#include <oscar/DOM/PropertyInfo.h>

#include <oscar/Variant/Variant.h>
#include <oscar/Variant/VariantType.h>

#include <gtest/gtest.h>

using namespace osc;

TEST(PropertyInfo, DefaultConstructedPropertyInfoHasNoNameAndNilDefaultValue)
{
    PropertyInfo info;
    ASSERT_EQ(info.name(), "");
    ASSERT_EQ(info.type(), VariantType::None);
    ASSERT_EQ(info.default_value(), Variant{});
}

TEST(PropertyInfo, CanConstructFromStringNameAndVariant)
{
    [[maybe_unused]] PropertyInfo const desc{"name", Variant{1.0f}};
}

TEST(PropertyInfo, GetNameReturnsSuppliedName)
{
    PropertyInfo const desc{"suppliedName", Variant{1.0f}};
    ASSERT_EQ(desc.name(), "suppliedName");
}

TEST(PropertyInfo, CanConstructFromStringRValue)
{
    ASSERT_NO_THROW({ PropertyInfo(std::string{"rvalue"}, Variant{false}); });
}

TEST(PropertyInfo, GetTypeReturnsSuppliedType)
{
    PropertyInfo const desc{"name", Variant{1.0f}};
    ASSERT_EQ(desc.type(), VariantType::Float);
}

TEST(PropertyInfo, ComparesEquivalentWhenGivenSameInformation)
{
    PropertyInfo const a{"name", Variant{1.0f}};
    PropertyInfo const b{"name", Variant{1.0f}};
    ASSERT_EQ(a, b);
}

TEST(PropertyInfo, ComparesInequivalentWhenGivenDifferentNames)
{
    PropertyInfo const a{"a", Variant{1.0f}};
    PropertyInfo const b{"b", Variant{1.0f}};
    ASSERT_NE(a, b);
}

TEST(PropertyInfo, ComparesInequivalentWhenGivenDifferentDefaultValues)
{
    PropertyInfo const a{"name", Variant{1.0f}};
    PropertyInfo const b{"name", Variant{2.0f}};
    ASSERT_NE(a, b);
}

TEST(PropertyInfo, ComparesInequivalentWhenGivenDifferentDefaultValueTypes)
{
    PropertyInfo const a{"name", Variant{1.0f}};
    PropertyInfo const b{"name", Variant{"different type"}};
    ASSERT_NE(a, b);
}

TEST(PropertyInfo, ThrowsAnExceptionWhenConstructedWithANameContainingWhitespace)
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

TEST(PropertyInfo, ThrowsWhenConstructedWithANameContainingAnyASCIIControlCharacters)
{
    auto const test = [](char c)
    {
        ASSERT_ANY_THROW({ PropertyInfo(std::string{c} + std::string("leading"), Variant(true)); });
        ASSERT_ANY_THROW({ PropertyInfo(std::string("trailing") + c, Variant(true)); });
        ASSERT_ANY_THROW({ PropertyInfo(std::string("inner") + c + std::string("usage"), Variant{true}); });
    };

    constexpr char c_LastControlCharacterInASCII = 0x1F;
    for (char c = 0; c <= c_LastControlCharacterInASCII; ++c)
    {
        test(c);
    }
    test(0x7F);  // DEL
}
