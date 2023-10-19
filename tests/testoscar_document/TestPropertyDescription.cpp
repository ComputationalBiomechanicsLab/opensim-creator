#include <oscar_document/PropertyDescription.hpp>

#include <oscar_document/VariantType.hpp>

#include <gtest/gtest.h>

TEST(PropertyDescription, CanConstructFromNameAndType)
{
    [[maybe_unused]] osc::doc::PropertyDescription const desc{"name", osc::doc::VariantType::Float};
}

TEST(PropertyDescription, GetNameReturnsSuppliedName)
{
    osc::doc::PropertyDescription const desc{"suppliedName", osc::doc::VariantType::Float};
    ASSERT_EQ(desc.getName(), "suppliedName");
}

TEST(PropertyDescription, GetTypeReturnsSuppliedType)
{
    osc::doc::PropertyDescription const desc{"name", osc::doc::VariantType::Float};
    ASSERT_EQ(desc.getType(), osc::doc::VariantType::Float);
}

TEST(PropertyDescription, ComparesEquivalentWhenGivenSameInformation)
{
    osc::doc::PropertyDescription const a{"name", osc::doc::VariantType::Float};
    osc::doc::PropertyDescription const b{"name", osc::doc::VariantType::Float};
    ASSERT_EQ(a, b);
}

TEST(PropertyDescription, ComparesInequivalentWhenGivenDifferentNames)
{
    osc::doc::PropertyDescription const a{"a", osc::doc::VariantType::Float};
    osc::doc::PropertyDescription const b{"b", osc::doc::VariantType::Float};
    ASSERT_NE(a, b);
}

TEST(PropertyDescription, ComparesInequivalentWhenGivenDifferentTypes)
{
    osc::doc::PropertyDescription const a{"name", osc::doc::VariantType::Float};
    osc::doc::PropertyDescription const b{"name", osc::doc::VariantType::Int};
    ASSERT_NE(a, b);
}
