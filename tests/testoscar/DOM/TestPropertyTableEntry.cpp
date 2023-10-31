#include <oscar/DOM/PropertyTableEntry.hpp>

#include <gtest/gtest.h>
#include <oscar/DOM/PropertyDescription.hpp>

using osc::PropertyDescription;
using osc::PropertyTableEntry;
using osc::Variant;

TEST(PropertyTableEntry, CanBeConstructedFromAPropertyDescription)
{
    PropertyDescription const desc{"name", Variant{"value"}};
    ASSERT_NO_THROW({ PropertyTableEntry{desc}; });
}

TEST(PropertyTableEntry, NameReturnsTheNameProvidedViaThePropertyDescription)
{
    PropertyDescription const desc{"name", Variant{"value"}};
    ASSERT_EQ(PropertyTableEntry{desc}.getName(), "name");
}

TEST(PropertyTableEntry, DefaultValueReturnsTheDefaultValueProvidedInTheDescription)
{
    PropertyDescription const desc{"name", Variant{1337}};
    ASSERT_EQ(PropertyTableEntry{desc}.getDefaultValue(), Variant{1337});
}

TEST(PropertyTableEntry, ValueInitiallyComparesEquivalentToTheProvidedDefaultValue)
{
    PropertyDescription const desc{"name", Variant{1337.0f}};
    ASSERT_EQ(PropertyTableEntry{desc}.getValue(), Variant{1337.0f});
}

TEST(PropertyTableEntry, SetValueWithCorrectTypeOfValueCausesGetValueToReturnNewValue)
{
    PropertyDescription const desc{"name", Variant{1337.0f}};
    PropertyTableEntry entry{desc};
    Variant const newValue{2.0f};

    ASSERT_EQ(entry.getValue(), desc.getDefaultValue());
    entry.setValue(newValue);
    ASSERT_EQ(entry.getValue(), newValue);
}

TEST(PropertyTableEntry, SetValueWithMismatchedTypeDoesNothing)
{
    PropertyDescription const desc{"name", Variant{1337.0f}};
    PropertyTableEntry entry{desc};
    Variant const invalidValue{"not a float"};

    ASSERT_EQ(entry.getValue(), desc.getDefaultValue());
    entry.setValue(invalidValue);
    ASSERT_NE(entry.getValue(), invalidValue);
    ASSERT_EQ(entry.getValue(), desc.getDefaultValue());
}
