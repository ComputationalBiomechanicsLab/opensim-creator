#include <oscar/DOM/Object.hpp>

#include <oscar/DOM/PropertyDescription.hpp>

#include <gtest/gtest.h>
#include <nonstd/span.hpp>

#include <memory>
#include <optional>
#include <random>
#include <string>
#include <string_view>

/*

using osc::Color;
using osc::Object;
using osc::PropertyDescription;
using osc::Variant;
using osc::VariantType;

namespace
{
    int RandomInt()
    {
        static auto s_PRNG = std::default_random_engine{std::random_device{}()};
        return std::uniform_int_distribution{}(s_PRNG);
    }

    class MinimalObjectImpl final : public Object {
    public:
        friend bool operator==(MinimalObjectImpl const& lhs, MinimalObjectImpl const& rhs)
        {
            return lhs.m_Data == rhs.m_Data;
        }
    private:
        std::unique_ptr<Object> implClone() const override
        {
            return std::make_unique<MinimalObjectImpl>(*this);
        }

        int m_Data = RandomInt();
    };

    class ObjectWithCustomToStringOverride final : public Object {
    public:
        ObjectWithCustomToStringOverride(std::string_view str) :
            m_ReturnedString{str}
        {
        }
    private:
        std::string implToString() const final
        {
            return m_ReturnedString;
        }

        std::unique_ptr<Object> implClone() const override
        {
            return std::make_unique<ObjectWithCustomToStringOverride>(*this);
        }

        std::string m_ReturnedString;
    };

    class ObjectWithGivenProperties final : public Object {
    public:
        ObjectWithGivenProperties(nonstd::span<PropertyDescription const> props) :
            Object{props}
        {
        }
    private:
        std::unique_ptr<Object> implClone() const override
        {
            return std::make_unique<ObjectWithGivenProperties>(*this);
        }
    };
}

TEST(Object, MinimalObjectImplCanDefaultConstruct)
{
    ASSERT_NO_THROW({ MinimalObjectImpl{}; });
}

TEST(Object, MinimalObjectImplCanCallToString)
{
    ASSERT_NO_THROW({ MinimalObjectImpl{}.toString(); });
}

TEST(Object, MinimalObjectImplToStringReturnsNonEmptyString)
{
    ASSERT_FALSE(MinimalObjectImpl{}.toString().empty());
}

TEST(Object, MinimalObjectImplCanClone)
{
    ASSERT_NO_THROW({ MinimalObjectImpl{}.clone(); });
}

TEST(Object, MinimalObjectImplCloneReturnsExpectedClone)
{
    MinimalObjectImpl const obj;
    ASSERT_EQ(dynamic_cast<MinimalObjectImpl const&>(*obj.clone()), obj);
}

TEST(Object, MinimalObjectImplGetNumPropertiesReturnsZero)
{
    ASSERT_EQ(MinimalObjectImpl{}.getNumProperties(), 0);
}

TEST(Object, MinimalObjectImplGetPropertyNameIsBoundsChecked)
{
    constexpr int c_OutOfBoundsIndex = 0;
    ASSERT_ANY_THROW({ MinimalObjectImpl{}.getPropertyName(c_OutOfBoundsIndex); });
}

TEST(Object, MinimalObjectImplGetPropertyIndexReturnsFalsey)
{
    ASSERT_FALSE(MinimalObjectImpl().getPropertyIndex("non-existent"));
}

TEST(Object, MinimalObjectImplTryGetPropertyDefaultValueReturnsFalsey)
{
    ASSERT_FALSE(MinimalObjectImpl{}.tryGetPropertyDefaultValue("non-existent"));
}

TEST(Object, MinimalObjectImplGetPropertyDefaultValueThrowsWithNonExistentName)
{
    ASSERT_ANY_THROW({ MinimalObjectImpl{}.getPropertyDefaultValue("non-existent"); });
}

TEST(Object, MinimalObjectImplTryGetPropertyValueReturnsFalsey)
{
    ASSERT_FALSE(MinimalObjectImpl().tryGetPropertyValue("non-existent"));
}

TEST(Object, MinimalObjectImplGetPropertyValueThrowsForNonExistentPropertyName)
{
    ASSERT_ANY_THROW({ MinimalObjectImpl{}.getPropertyValue("non-existent"); });
}

TEST(Object, MinimalObjectImplTrySetPropertyValueReturnsFalseyForNonExistentPropertyName)
{
    ASSERT_FALSE(MinimalObjectImpl().trySetPropertyValue("non-existent", Variant{true}));
}

TEST(Object, MinimalObjectImplSetPropertyValueThrowsForNonExistentPropertyName)
{
    ASSERT_ANY_THROW({ MinimalObjectImpl{}.setPropertyValue("non-existent", Variant{false}); });
}

TEST(Object, MinimalObjectToStringFreeFunctionReturnsSameAsToStringMemberFunction)
{
    MinimalObjectImpl const obj;
    ASSERT_EQ(to_string(obj), obj.toString());
}

TEST(Object, ObjectWithCustomToStringOverrideReturnsExpectedString)
{
    std::string_view const expected = "some-injected-string";
    ASSERT_EQ(ObjectWithCustomToStringOverride{expected}.toString(), expected);
    ASSERT_EQ(to_string(ObjectWithCustomToStringOverride{expected}), expected);
}

TEST(Object, ObjectWithGivenPropertiesWhenClonedReturnsSameNumProperties)
{
    auto const descriptions = osc::to_array<PropertyDescription>(
    {
        {"intprop", Variant{20}},
        {"floatprop", Variant{10.0f}},
        {"stringprop", Variant{"str"}},
    });
    ObjectWithGivenProperties const orig{descriptions};
    auto const clone = static_cast<Object const&>(orig).clone();
    ASSERT_EQ(orig.getNumProperties(), clone->getNumProperties());
}

TEST(Object, ObjectWithGivenPropertiesWhenClonedHasPropertyNamesInSameOrder)
{
    auto const descriptions = osc::to_array<PropertyDescription>(
    {
        {"intprop", Variant{20}},
        {"floatprop", Variant{10.0f}},
        {"stringprop", Variant{"str"}},
    });
    ObjectWithGivenProperties const orig{descriptions};
    auto const clone = static_cast<Object const&>(orig).clone();

    ASSERT_EQ(orig.getNumProperties(), clone->getNumProperties());
    for (size_t i = 0; i < orig.getNumProperties(); ++i)
    {
        ASSERT_EQ(orig.getPropertyName(i), clone->getPropertyName(i));
    }
}

TEST(Object, ObjectWithGivenPropertiesWhenClonedHasSamePropertyValues)
{
    auto const descriptions = osc::to_array<PropertyDescription>(
    {
        {"intprop", Variant{20}},
        {"floatprop", Variant{10.0f}},
        {"stringprop", Variant{"str"}},
    });
    auto const newIntValue = Variant{40};

    ObjectWithGivenProperties orig{descriptions};
    orig.setPropertyValue("intprop", newIntValue);
    ASSERT_EQ(orig.getPropertyValue("intprop"), newIntValue);

    auto const clone = static_cast<Object const&>(orig).clone();
    ASSERT_EQ(clone->getPropertyValue("intprop"), newIntValue);
}

TEST(Object, ObjectWithGivenPropertiesWhenClonedHasSamePropertyNames)
{
    auto const descriptions = osc::to_array<PropertyDescription>(
    {
        {"intprop", Variant{20}},
        {"floatprop", Variant{10.0f}},
        {"stringprop", Variant{"str"}},
    });
    ObjectWithGivenProperties const orig{descriptions};
    auto const clone = static_cast<Object const&>(orig).clone();

    for (PropertyDescription const& description : descriptions)
    {
        ASSERT_TRUE(clone->getPropertyIndex(description.getName()));
    }
}

TEST(Object, ObjectWithGivenPropertiesWhenClonedHasSameDefaultValues)
{
    auto const descriptions = osc::to_array<PropertyDescription>(
    {
        {"intprop", Variant{20}},
        {"floatprop", Variant{10.0f}},
        {"stringprop", Variant{"str"}},
    });
    ObjectWithGivenProperties const orig{descriptions};
    auto const clone = static_cast<Object const&>(orig).clone();

    for (PropertyDescription const& description : descriptions)
    {
        ASSERT_EQ(clone->getPropertyDefaultValue(description.getName()), description.getDefaultValue());
    }
}

TEST(Object, GetNumPropertiesReturnsZeroWhenProvidedEmptySpan)
{
    std::array<PropertyDescription, 0> descriptions{};
    ObjectWithGivenProperties const a{descriptions};
    ASSERT_EQ(a.getNumProperties(), 0);
}

TEST(Object, GetNumPropertiesReturnsOneWhenProvidedOneProperty)
{
    auto const descriptions = osc::to_array<PropertyDescription>(
    {
        {"someprop", Variant{false}},
    });
    ObjectWithGivenProperties const a{descriptions};
    ASSERT_EQ(a.getNumProperties(), 1);
}

TEST(Object, GetNumPropertiesReturnsTwoWhenProvidedTwoProperties)
{
    auto const descriptions = osc::to_array<PropertyDescription>(
    {
        {"someprop", Variant{false}},
        {"somecolor", Variant{Color::red()}},
    });
    ObjectWithGivenProperties const a{descriptions};
    ASSERT_EQ(a.getNumProperties(), 2);
}

TEST(Object, GetNumPropertiesReturnsThreeWhenProvidedThreeProperties)
{
    auto const descriptions = osc::to_array<PropertyDescription>(
    {
        {"someprop", Variant{false}},
        {"somecolor", Variant{Color::red()}},
        {"somestring", Variant{"boring"}},
    });
    ObjectWithGivenProperties const a{descriptions};
    ASSERT_EQ(a.getNumProperties(), 3);
}

TEST(Object, GetNumPropertiesReturnsTwoWhenProvidedThreePropertyDescriptionsWithADuplicatedName)
{
    auto const descriptions = osc::to_array<PropertyDescription>(
    {
        {"a", Variant{false}},
        {"b", Variant{Color::red()}},
        {"a", Variant{"boring"}},
    });
    ObjectWithGivenProperties const a{descriptions};
    ASSERT_EQ(a.getNumProperties(), 2);
}

TEST(Object, GetNumPropertiesReturnsTheFirstProvidedPropertyDescriptionWhenGivenDuplicateNames)
{
    auto const descriptions = osc::to_array<PropertyDescription>(
    {
        {"a", Variant{false}},
        {"b", Variant{Color::red()}},
        {"a", Variant{"boring"}},
    });
    ObjectWithGivenProperties const a{descriptions};
    ASSERT_TRUE(a.getPropertyValue("a").getType() == VariantType::Bool);
}

TEST(Object, GetPropertyNameReturnsPropertiesInTheProvidedOrder)
{
    auto const descriptions = osc::to_array<PropertyDescription>(
    {
        {"a", Variant{false}},
        {"b", Variant{false}},
        {"c", Variant{false}},
        {"d", Variant{false}},
        {"e", Variant{false}},
        {"f", Variant{false}},
        {"g", Variant{false}},
        {"h", Variant{false}},
    });
    ObjectWithGivenProperties const a{descriptions};

    ASSERT_EQ(a.getNumProperties(), descriptions.size());
    for (size_t i = 0; i < descriptions.size(); ++i)
    {
        ASSERT_EQ(a.getPropertyName(i), descriptions.at(i).getName());
    }
}

TEST(Object, GetPropertyIndexReturnsNulloptForInvalidName)
{
    auto const descriptions = osc::to_array<PropertyDescription>(
    {
        {"a", Variant{false}},
    });
    ObjectWithGivenProperties const a{descriptions};
    ASSERT_EQ(a.getPropertyIndex("non-existent"), std::nullopt);
}

TEST(Object, GetPropertyIndexReturnsExpectedIndexForCorrectName)
{
    auto const descriptions = osc::to_array<PropertyDescription>(
    {
        {"a", Variant{false}},
        {"b", Variant{false}},
    });
    ObjectWithGivenProperties const a{descriptions};
    ASSERT_EQ(a.getPropertyIndex("a"), 0);
    ASSERT_EQ(a.getPropertyIndex("b"), 1);
}

TEST(Object, TryGetPropertyDefaultValueReturnsNullptrForInvalidName)
{
    auto const descriptions = osc::to_array<PropertyDescription>(
    {
        {"a", Variant{false}},
        {"b", Variant{false}},
    });
    ObjectWithGivenProperties const a{descriptions};
    ASSERT_EQ(a.tryGetPropertyDefaultValue("non-existent"), nullptr);
}

TEST(Object, TryGetPropertyDefaultValueReturnsNonNullptrForCorrectName)
{
    auto const descriptions = osc::to_array<PropertyDescription>(
    {
        {"a", Variant{false}},
        {"b", Variant{false}},
    });
    ObjectWithGivenProperties const a{descriptions};
    ASSERT_NE(a.tryGetPropertyDefaultValue("a"), nullptr);
    ASSERT_NE(a.tryGetPropertyDefaultValue("b"), nullptr);
}

TEST(Object, TryGetPropertyDefaultValueReturnsNonNullptrToCorrectValueForCorrectName)
{
    auto const descriptions = osc::to_array<PropertyDescription>(
    {
        {"a", Variant{1337}},
        {"b", Variant{-1}},
    });
    ObjectWithGivenProperties const a{descriptions};
    ASSERT_NE(a.tryGetPropertyDefaultValue("a"), nullptr);
    ASSERT_EQ(*a.tryGetPropertyDefaultValue("a"), Variant{1337});
}

TEST(Object, GetPropertyDefaultValueThrowsForInvalidName)
{
    auto const descriptions = osc::to_array<PropertyDescription>(
    {
        {"a", Variant{1337}},
        {"b", Variant{-1}},
    });
    ObjectWithGivenProperties const a{descriptions};
    ASSERT_ANY_THROW({ a.getPropertyDefaultValue("non-existent"); });
}

TEST(Object, GetPropertyDefaultValueDoesNotThrowForCorrectName)
{
    auto const descriptions = osc::to_array<PropertyDescription>(
    {
        {"a", Variant{1337}},
        {"b", Variant{-1}},
    });
    ObjectWithGivenProperties const a{descriptions};
    ASSERT_EQ(a.getPropertyDefaultValue("a"), Variant{1337});
}

TEST(Object, TryGetPropertyValueReturnsNullptrForInvalidName)
{
    auto const descriptions = osc::to_array<PropertyDescription>(
    {
        {"a", Variant{false}},
        {"b", Variant{false}},
    });
    ObjectWithGivenProperties const a{descriptions};
    ASSERT_EQ(a.tryGetPropertyValue("non-existent"), nullptr);
}

TEST(Object, TryGetPropertyValueReturnsNonNullptrForValidName)
{
    auto const descriptions = osc::to_array<PropertyDescription>(
    {
        {"a", Variant{false}},
        {"b", Variant{false}},
    });
    ObjectWithGivenProperties const a{descriptions};
    ASSERT_NE(a.tryGetPropertyValue("a"), nullptr);
    ASSERT_NE(a.tryGetPropertyValue("b"), nullptr);
}

TEST(Object, TryGetPropertyValueReturnsDefaultValueWhenValueHasNotBeenSet)
{
    auto const descriptions = osc::to_array<PropertyDescription>(
    {
        {"a", Variant{1337}},
        {"b", Variant{false}},
    });
    ObjectWithGivenProperties const a{descriptions};
    ASSERT_TRUE(a.tryGetPropertyValue("a"));
    ASSERT_EQ(*a.tryGetPropertyValue("a"), Variant{1337});
}

TEST(Object, TryGetPropertyValueReturnsNewValueAfterTheValueHasBeenSet)
{
    auto const oldValue = Variant{10};
    auto const newValue = Variant{50};
    auto const descriptions = osc::to_array<PropertyDescription>(
    {
        {"a", Variant{false}},
        {"b", oldValue},
    });

    ObjectWithGivenProperties a{descriptions};
    ASSERT_TRUE(a.tryGetPropertyValue("b"));
    ASSERT_EQ(*a.tryGetPropertyValue("b"), oldValue);
    a.trySetPropertyValue("b", newValue);
    ASSERT_TRUE(a.tryGetPropertyValue("b"));
    ASSERT_EQ(*a.tryGetPropertyValue("b"), newValue);
}

TEST(Object, GetPropertyValueThrowsIfGivenNonExistentPropertyName)
{
    auto const descriptions = osc::to_array<PropertyDescription>(
    {
        {"a", Variant{false}},
        {"b", Variant{false}},
    });
    ObjectWithGivenProperties const a{descriptions};
    ASSERT_ANY_THROW({ a.getPropertyValue("a"); });
}

TEST(Object, GetPropertyValueReturnsValueIfGivenExistentPropertyName)
{
    auto const descriptions = osc::to_array<PropertyDescription>(
    {
        {"a", Variant{false}},
        {"b", Variant{"second"}},
    });
    ObjectWithGivenProperties const a{descriptions};
    ASSERT_EQ(a.getPropertyValue("a"), Variant{"second"});
}

TEST(Object, GetPropertyValueReturnsNewValueAfterSettingValue)
{
    auto const oldValue = Variant{"old"};
    auto const newValue = Variant{"new"};
    auto const descriptions = osc::to_array<PropertyDescription>(
    {
        {"a", Variant{false}},
        {"b", oldValue},
    });

    ObjectWithGivenProperties a{descriptions};
    ASSERT_EQ(a.getPropertyValue("b"), oldValue);
    a.trySetPropertyValue("b", newValue);
    ASSERT_EQ(a.getPropertyValue("b"), newValue);
}

// TODO: trySetPropertyValue
// TODO: setPropertyValue

// TODO: with implCustomPropertyGetter:
//
// - tryGetPropertyValue returns getter value if non-nullptr
// - getPropertyValue returns getter value if non-nullptr
// - tryGetPropertyValue returns "normal" value if nullptr
// - getPropertyValue returns "normal" value if nullptr

*/
