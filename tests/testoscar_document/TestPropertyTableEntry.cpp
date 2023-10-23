#include <oscar_document/PropertyTableEntry.hpp>

#include <gtest/gtest.h>
#include <oscar_document/PropertyDescription.hpp>

using osc::doc::PropertyDescription;
using osc::doc::PropertyTableEntry;
using osc::doc::Variant;

TEST(PropertyTableEntry, CanBeConstructedFromAPropertyDescription)
{
    PropertyDescription const desc{"name", Variant{"value"}};
    ASSERT_NO_THROW({ PropertyTableEntry{desc}; });
}

TEST(PropertyTableEntry, NameReturnsTheNameProvidedViaThePropertyDescription)
{
    PropertyDescription const desc{"name", Variant{"value"}};
    ASSERT_EQ(PropertyTableEntry{desc}.name(), "name");
}

TEST(PropertyTableEntry, DefaultValueReturnsTheDefaultValueProvidedInTheDescription)
{
    PropertyDescription const desc{"name", Variant{1337}};
    ASSERT_EQ(PropertyTableEntry{desc}.defaultValue(), Variant{1337});
}

TEST(PropertyTableEntry, ValueInitiallyComparesEquivalentToTheProvidedDefaultValue)
{
    PropertyDescription const desc{"name", Variant{1337.0f}};
    ASSERT_EQ(PropertyTableEntry{desc}.value(), Variant{1337.0f});
}

TEST(PropertyTableEntry, SetValueWithCorrectTypeOfValueCausesGetValueToReturnNewValue)
{
    PropertyDescription const desc{"name", Variant{1337.0f}};
    PropertyTableEntry entry{desc};
    Variant const newValue{2.0f};

    ASSERT_EQ(entry.value(), desc.getDefaultValue());
    entry.setValue(newValue);
    ASSERT_EQ(entry.value(), newValue);
}

TEST(PropertyTableEntry, SetValueWithMismatchedTypeDoesNothing)
{
    PropertyDescription const desc{"name", Variant{1337.0f}};
    PropertyTableEntry entry{desc};
    Variant const invalidValue{"not a float"};

    ASSERT_EQ(entry.value(), desc.getDefaultValue());
    entry.setValue(invalidValue);
    ASSERT_NE(entry.value(), invalidValue);
    ASSERT_EQ(entry.value(), desc.getDefaultValue());
}
