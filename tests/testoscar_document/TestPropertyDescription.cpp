#include <oscar_document/PropertyDescription.hpp>

#include <oscar_document/Variant.hpp>
#include <oscar_document/VariantType.hpp>

#include <gtest/gtest.h>

TEST(PropertyDescription, CanConstructFromStringNameAndVariant)
{
    [[maybe_unused]] osc::doc::PropertyDescription const desc{"name", osc::doc::Variant{1.0f}};
}

TEST(PropertyDescription, GetNameReturnsSuppliedName)
{
    osc::doc::PropertyDescription const desc{"suppliedName", osc::doc::Variant{1.0f}};
    ASSERT_EQ(desc.getName(), "suppliedName");
}

TEST(PropertyDescription, GetTypeReturnsSuppliedType)
{
    osc::doc::PropertyDescription const desc{"name", osc::doc::Variant{1.0f}};
    ASSERT_EQ(desc.getType(), osc::doc::VariantType::Float);
}

TEST(PropertyDescription, ComparesEquivalentWhenGivenSameInformation)
{
    osc::doc::PropertyDescription const a{"name", osc::doc::Variant{1.0f}};
    osc::doc::PropertyDescription const b{"name", osc::doc::Variant{1.0f}};
    ASSERT_EQ(a, b);
}

TEST(PropertyDescription, ComparesInequivalentWhenGivenDifferentNames)
{
    osc::doc::PropertyDescription const a{"a", osc::doc::Variant{1.0f}};
    osc::doc::PropertyDescription const b{"b", osc::doc::Variant{1.0f}};
    ASSERT_NE(a, b);
}

TEST(PropertyDescription, ComparesInequivalentWhenGivenDifferentDefaultValues)
{
    osc::doc::PropertyDescription const a{"name", osc::doc::Variant{1.0f}};
    osc::doc::PropertyDescription const b{"name", osc::doc::Variant{2.0f}};
    ASSERT_NE(a, b);
}

TEST(PropertyDescription, ComparesInequivalentWhenGivenDifferentDefaultValueTypes)
{
    osc::doc::PropertyDescription const a{"name", osc::doc::Variant{1.0f}};
    osc::doc::PropertyDescription const b{"name", osc::doc::Variant{"different type"}};
    ASSERT_NE(a, b);
}
