#include <oscar_document/PropertyDescriptions.hpp>

#include <gtest/gtest.h>
#include <oscar/Utils/Cpp20Shims.hpp>
#include <oscar_document/PropertyDescription.hpp>

#include <algorithm>
#include <iterator>

TEST(PropertyDescriptions, CanBeDefaultConstructed)
{
    [[maybe_unused]] osc::doc::PropertyDescriptions descs;
}

TEST(PropertyDescriptions, CanAppendAPropertyDescription)
{
    osc::doc::PropertyDescriptions descs;
    descs.append(osc::doc::PropertyDescription{"name", osc::doc::VariantType::Float});
}

TEST(PropertyDescriptions, AppendingTwoPropertiesWithTheSameNameThrows)
{
    osc::doc::PropertyDescriptions descs;
    descs.append(osc::doc::PropertyDescription{"name", osc::doc::VariantType::Float});
    ASSERT_ANY_THROW({ descs.append(osc::doc::PropertyDescription{"name", osc::doc::VariantType::Float}); });
}

TEST(PropertyDescriptions, SizeReturnsZeroOnDefaultConstruction)
{
    osc::doc::PropertyDescriptions descs;
    ASSERT_EQ(descs.size(), 0);
}

TEST(PropertyDescriptions, SizeReturnsOneAfterAppendingAPropertyDescription)
{
    osc::doc::PropertyDescriptions descs;
    descs.append(osc::doc::PropertyDescription{"name", osc::doc::VariantType::Float});
    ASSERT_EQ(descs.size(), 1);
}

TEST(PropertyDescriptions, SizeReturnsTwoAfterAppendingTwoDescriptions)
{
    osc::doc::PropertyDescriptions descs;
    descs.append(osc::doc::PropertyDescription{"first", osc::doc::VariantType::Float});
    descs.append(osc::doc::PropertyDescription{"second", osc::doc::VariantType::Float});
    ASSERT_EQ(descs.size(), 2);
}

TEST(PropertyDescriptions, AtThrowsIfGivenZeroIndexToEmptyVersion)
{
    osc::doc::PropertyDescriptions descs;
    ASSERT_ANY_THROW({ descs.at(0); });
}

TEST(PropertyDescriptions, AtReturnsNthElement)
{
    osc::doc::PropertyDescriptions descs;
    osc::doc::PropertyDescription desc{"name", osc::doc::VariantType::Float};

    descs.append(desc);
    ASSERT_EQ(descs.at(0), desc);
}

TEST(PropertyDescriptions, AtThrowsWhenOOBWithOneElement)
{
    osc::doc::PropertyDescriptions descs;

    descs.append(osc::doc::PropertyDescription{"name", osc::doc::VariantType::Float});
    ASSERT_ANY_THROW({ descs.at(1); });
}

TEST(PropertyDescriptions, BeginEqualsEndForEmpty)
{
    osc::doc::PropertyDescriptions const descs;
    ASSERT_EQ(descs.begin(), descs.end());
}

TEST(PropertyDescriptions, BeginDoesntEqualEndForOccupiedCollection)
{
    osc::doc::PropertyDescriptions descs;
    descs.append(osc::doc::PropertyDescription{"name", osc::doc::VariantType::Float});

    ASSERT_NE(descs.begin(), descs.end());
}

TEST(PropertyDescriptions, IteratorsReturnExpectedDistance)
{
    osc::doc::PropertyDescriptions descs;
    descs.append(osc::doc::PropertyDescription{"a", osc::doc::VariantType::Float});
    descs.append(osc::doc::PropertyDescription{"b", osc::doc::VariantType::Float});

    ASSERT_EQ(std::distance(descs.begin(), descs.end()), 2);
}

TEST(PropertyDescriptions, IteratorsReturnExpectedValues)
{
    auto const values = osc::to_array<osc::doc::PropertyDescription>(
    {
        {"a", osc::doc::VariantType::Float},
        {"b", osc::doc::VariantType::Int},
        {"c", osc::doc::VariantType::Float},
    });

    osc::doc::PropertyDescriptions descs;
    for (auto const& value : values)
    {
        descs.append(value);
    }

    ASSERT_TRUE(std::equal(descs.begin(), descs.end(), values.begin(), values.end()));
}
