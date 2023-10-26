#include <oscar_document/PropertyDescription.hpp>

#include <oscar_document/Variant.hpp>
#include <oscar_document/VariantType.hpp>

#include <gtest/gtest.h>

using osc::doc::PropertyDescription;
using osc::doc::Variant;
using osc::doc::VariantType;


TEST(PropertyDescription, CanConstructFromStringNameAndVariant)
{
    [[maybe_unused]] PropertyDescription const desc{"name", Variant{1.0f}};
}

TEST(PropertyDescription, GetNameReturnsSuppliedName)
{
    PropertyDescription const desc{"suppliedName", Variant{1.0f}};
    ASSERT_EQ(desc.getName(), "suppliedName");
}

TEST(PropertyDescription, CanConstructFromStringRValue)
{
    ASSERT_NO_THROW({ PropertyDescription(std::string{"rvalue"}, Variant{false}); });
}

TEST(PropertyDescription, GetTypeReturnsSuppliedType)
{
    PropertyDescription const desc{"name", Variant{1.0f}};
    ASSERT_EQ(desc.getType(), VariantType::Float);
}

TEST(PropertyDescription, ComparesEquivalentWhenGivenSameInformation)
{
    PropertyDescription const a{"name", Variant{1.0f}};
    PropertyDescription const b{"name", Variant{1.0f}};
    ASSERT_EQ(a, b);
}

TEST(PropertyDescription, ComparesInequivalentWhenGivenDifferentNames)
{
    PropertyDescription const a{"a", Variant{1.0f}};
    PropertyDescription const b{"b", Variant{1.0f}};
    ASSERT_NE(a, b);
}

TEST(PropertyDescription, ComparesInequivalentWhenGivenDifferentDefaultValues)
{
    PropertyDescription const a{"name", Variant{1.0f}};
    PropertyDescription const b{"name", Variant{2.0f}};
    ASSERT_NE(a, b);
}

TEST(PropertyDescription, ComparesInequivalentWhenGivenDifferentDefaultValueTypes)
{
    PropertyDescription const a{"name", Variant{1.0f}};
    PropertyDescription const b{"name", Variant{"different type"}};
    ASSERT_NE(a, b);
}

TEST(PropertyDescription, ThrowsAnExceptionWhenConstructedWithANameContainingWhitespace)
{
    // (basic examples)

    ASSERT_ANY_THROW({ PropertyDescription(" leadingSpace", Variant{true}); });
    ASSERT_ANY_THROW({ PropertyDescription("trailingSpace ", Variant{true}); });
    ASSERT_ANY_THROW({ PropertyDescription("inner space", Variant{true}); });

    ASSERT_ANY_THROW({ PropertyDescription("\nleadingNewline", Variant{true}); });
    ASSERT_ANY_THROW({ PropertyDescription("trailingNewline\n", Variant{true}); });
    ASSERT_ANY_THROW({ PropertyDescription("inner\newline", Variant{true}); });

    ASSERT_ANY_THROW({ PropertyDescription("\tleadingTab", Variant{true}); });
    ASSERT_ANY_THROW({ PropertyDescription("trailingTab\t", Variant{true}); });
    ASSERT_ANY_THROW({ PropertyDescription("inner\tTab", Variant{true}); });
}

TEST(PropertyDescription, ThrowsWhenConstructedWithANameContainingAnyASCIIControlCharacters)
{
    auto const test = [](char c)
    {
        ASSERT_ANY_THROW({ PropertyDescription(std::string{c} + std::string("leading"), Variant(true)); });
        ASSERT_ANY_THROW({ PropertyDescription(std::string("trailing") + c, Variant(true)); });
        ASSERT_ANY_THROW({ PropertyDescription(std::string("inner") + c + std::string("usage"), Variant{true}); });
    };

    constexpr char c_LastControlCharacterInASCII = 0x1F;
    for (char c = 0; c <= c_LastControlCharacterInASCII; ++c)
    {
        test(c);
    }
    test(0x7F);  // DEL
}
