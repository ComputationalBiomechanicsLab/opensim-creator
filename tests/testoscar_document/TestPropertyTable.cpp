#include <oscar_document/PropertyTable.hpp>

#include <gtest/gtest.h>
#include <oscar/Utils/Cpp20Shims.hpp>
#include <oscar_document/PropertyDescription.hpp>
#include <oscar_document/PropertyTableEntry.hpp>
#include <oscar_document/StringName.hpp>
#include <oscar_document/Variant.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>
#include <tuple>
#include <unordered_set>

using osc::doc::PropertyDescription;
using osc::doc::PropertyTable;
using osc::doc::Variant;

namespace
{
    auto const& GetPropertyDescriptionFixture()
    {
        static auto const s_Entries = osc::to_array<PropertyDescription>(
        {
            {"val1", Variant{1}},
            {"val2", Variant{"2"}},
            {"val3", Variant{100.0f}},
            {"val4", Variant{false}},
        });
        return s_Entries;
    }

    auto const& GetPropertyDescriptionsWithDuplicateNamesFixture()
    {
        static auto const s_Entries = osc::to_array<PropertyDescription>(
        {
            {"val1", Variant{"first occurance"}},
            {"val2", Variant{"2"}},
            {"val3", Variant{100.0f}},
            {"val1", Variant{"second occurance"}},
            {"val4", Variant{false}},
            {"val4", Variant{-20.0f}},
        });
        return s_Entries;
    }
}

TEST(PropertyTable, CanBeDefaultConstructed)
{
    ASSERT_NO_THROW({ PropertyTable{}; });
}

TEST(PropertyTable, HasZeroSizeWhenDefaultConstructed)
{
    ASSERT_EQ(PropertyTable{}.size(), 0);
}

TEST(PropertyTable, CanBeConstructedFromASequenceOfPropertyDescriptions)
{
    ASSERT_NO_THROW({ PropertyTable{GetPropertyDescriptionFixture()}; });
}

TEST(PropertyTable, HasSameSizeAsProvidedNumberOfDescriptions)
{
    ASSERT_EQ(PropertyTable{GetPropertyDescriptionFixture()}.size(), GetPropertyDescriptionFixture().size());
}

TEST(PropertyTable, EachElementIsInTheSameOrderAsTheProvidedDescriptions)
{
    auto const& descriptions = GetPropertyDescriptionFixture();
    PropertyTable const table{descriptions};

    ASSERT_EQ(table.size(), descriptions.size());
    for (size_t i = 0; i < descriptions.size(); ++i)
    {
        ASSERT_EQ(table[i].name(), descriptions[i].getName());
        ASSERT_EQ(table[i].value(), descriptions[i].getDefaultValue());
        ASSERT_EQ(table[i].defaultValue(), descriptions[i].getDefaultValue());
    }
}

TEST(PropertyTable, IndexOfReturnsCorrectIndexForGivenName)
{
    auto const& descriptions = GetPropertyDescriptionFixture();
    PropertyTable const table{descriptions};

    for (size_t i = 0; i < descriptions.size(); ++i)
    {
        auto const rv = table.indexOf(descriptions[i].getName());
        ASSERT_TRUE(rv);
        ASSERT_EQ(*rv, i);
        ASSERT_EQ(table[*rv].name(), descriptions[i].getName());
    }
}

TEST(PropertyTable, IndexOfReturnsFalseyForNonExistentPropertyName)
{
    auto const& descriptions = GetPropertyDescriptionFixture();
    PropertyTable const table{descriptions};

    ASSERT_FALSE(table.indexOf("non-existent"));
}

TEST(PropertyTable, SetValueSetsPropertyValueIfTypesMatch)
{
    auto const& descriptions = GetPropertyDescriptionFixture();
    PropertyTable table{descriptions};

    auto const newValues = osc::to_array<Variant>(
    {
        Variant{-5},
        Variant{"5"},
        Variant{-400.0f},
        Variant{true},
    });

    ASSERT_EQ(newValues.size(), table.size());
    ASSERT_EQ(newValues.size(), descriptions.size());
    for (size_t i = 0; i < newValues.size(); ++i)
    {
        ASSERT_EQ(table[i].value(), descriptions[i].getDefaultValue());
        ASSERT_EQ(table[i].value().getType(), newValues[i].getType());
        ASSERT_NE(table[i].value(), newValues[i]);
        table.setValue(i, newValues[i]);
        ASSERT_EQ(table[i].value(), newValues[i]);
    }
}

TEST(PropertyTable, SetValueDoesNothingIfTypesMismatch)
{
    auto const& descriptions = GetPropertyDescriptionFixture();
    PropertyTable table{descriptions};

    auto const newValues = osc::to_array<Variant>(
    {
        Variant{"not an int"},
        Variant{true},
        Variant{"not a float"},
        Variant{1337},
    });

    ASSERT_EQ(newValues.size(), table.size());
    ASSERT_EQ(newValues.size(), descriptions.size());
    for (size_t i = 0; i < newValues.size(); ++i)
    {
        ASSERT_EQ(table[i].value(), descriptions[i].getDefaultValue());
        ASSERT_NE(table[i].value().getType(), newValues[i].getType());
        ASSERT_NE(table[i].value(), newValues[i]);
        table.setValue(i, newValues[i]);
        ASSERT_NE(table[i].value(), newValues[i]);
        ASSERT_EQ(table[i].value(), descriptions[i].getDefaultValue());  // i.e. nothing changed
    }
}

TEST(PropertyTable, IfGivenPropertiesWithDuplicateNamesTakesTheLatestDuplicateInTheProvidedVector)
{
    auto const& descriptions = GetPropertyDescriptionsWithDuplicateNamesFixture();
    PropertyTable const table{descriptions};

    std::unordered_set<std::string> uniqueNames;
    for (auto const& desc : descriptions)
    {
        uniqueNames.insert(std::string{desc.getName()});
    }

    ASSERT_NE(table.size(), descriptions.size());
    ASSERT_EQ(table.size(), uniqueNames.size());

    for (auto const& uniqueName : uniqueNames)
    {
        auto const hasName = [uniqueName](auto const& desc)
        {
            return desc.getName() == uniqueName;
        };
        auto const itFromEnd = std::find_if(descriptions.rbegin(), descriptions.rend(), hasName);
        ASSERT_NE(itFromEnd, descriptions.rend());
        auto const secondItFromEnd = std::find_if(itFromEnd, descriptions.rend(), hasName);

        if (secondItFromEnd != descriptions.rend())
        {
            // i.e. it's one of the non-unique entries in the input descriptions
            auto idx = table.indexOf(itFromEnd->getName());
            ASSERT_TRUE(idx);
            ASSERT_EQ(table[*idx].defaultValue(), itFromEnd->getDefaultValue());
        }
    }
}
