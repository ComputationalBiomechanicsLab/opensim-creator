#include <oscar/DOM/Object.h>

/*

#include <oscar/Object/PropertyInfo.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Utils/StringName.h>
#include <oscar/Variant/Variant.h>
#include <oscar/Variant/VariantType.h>

#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <random>
#include <span>
#include <string>
#include <string_view>

using osc::Color;
using osc::Object;
using osc::PropertyInfo;
using osc::SetPropertyStrategy;
using osc::StringName;
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
        std::unique_ptr<Object> impl_clone() const override
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
        std::string impl_to_string() const final
        {
            return m_ReturnedString;
        }

        std::unique_ptr<Object> impl_clone() const override
        {
            return std::make_unique<ObjectWithCustomToStringOverride>(*this);
        }

        std::string m_ReturnedString;
    };

    class ObjectWithGivenProperties final : public Object {
    public:
        ObjectWithGivenProperties(std::span<PropertyInfo const> props) :
            Object{props}
        {
        }
    private:
        std::unique_ptr<Object> impl_clone() const override
        {
            return std::make_unique<ObjectWithGivenProperties>(*this);
        }
    };

    class ObjectWithcustomSetPropertyStrategy final : public Object {
    public:
        ObjectWithcustomSetPropertyStrategy(
            std::span<PropertyInfo const> propertyDescriptions_,
            SetPropertyStrategy strategy_) :

            Object{propertyDescriptions_},
            m_Strategy{std::move(strategy_)}
        {
        }
    private:
        std::unique_ptr<Object> impl_clone() const override
        {
            return std::make_unique<ObjectWithcustomSetPropertyStrategy>(*this);
        }

        SetPropertyStrategy implSetPropertyStrategy(StringName const&, Variant const&)
        {
            return m_Strategy;
        }

        SetPropertyStrategy m_Strategy;
    };
}

TEST(Object, MinimalObjectImplCanDefaultConstruct)
{
    ASSERT_NO_THROW({ MinimalObjectImpl{}; });
}

TEST(Object, MinimalObjectImplCanCallToString)
{
    ASSERT_NO_THROW({ MinimalObjectImpl{}.to_string(); });
}

TEST(Object, MinimalObjectImplToStringReturnsNonEmptyString)
{
    ASSERT_FALSE(MinimalObjectImpl{}.to_string().empty());
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
    ASSERT_EQ(MinimalObjectImpl{}.num_properties(), 0);
}

TEST(Object, MinimalObjectImplGetPropertyNameIsBoundsChecked)
{
    constexpr int c_OutOfBoundsIndex = 0;
    ASSERT_ANY_THROW({ MinimalObjectImpl{}.getPropertyName(c_OutOfBoundsIndex); });
}

TEST(Object, MinimalObjectImplGetPropertyIndexReturnsFalsey)
{
    ASSERT_FALSE(MinimalObjectImpl().property_index("non-existent"));
}

TEST(Object, MinimalObjectImplTryGetPropertyDefaultValueReturnsFalsey)
{
    ASSERT_FALSE(MinimalObjectImpl{}.property_default_value("non-existent"));
}

TEST(Object, MinimalObjectImplGetPropertyDefaultValueThrowsWithNonExistentName)
{
    ASSERT_ANY_THROW({ MinimalObjectImpl{}.property_default_value_or_throw("non-existent"); });
}

TEST(Object, MinimalObjectImplTryGetPropertyValueReturnsFalsey)
{
    ASSERT_FALSE(MinimalObjectImpl().property_value("non-existent"));
}

TEST(Object, MinimalObjectImplGetPropertyValueThrowsForNonExistentPropertyName)
{
    ASSERT_ANY_THROW({ MinimalObjectImpl{}.property_value_or_throw("non-existent"); });
}

TEST(Object, MinimalObjectImplTrySetPropertyValueReturnsFalseyForNonExistentPropertyName)
{
    ASSERT_FALSE(MinimalObjectImpl().set_property_value("non-existent", Variant{true}));
}

TEST(Object, MinimalObjectImplSetPropertyValueThrowsForNonExistentPropertyName)
{
    ASSERT_ANY_THROW({ MinimalObjectImpl{}.set_property_value_or_throw("non-existent", Variant{false}); });
}

TEST(Object, MinimalObjectToStringFreeFunctionReturnsSameAsToStringMemberFunction)
{
    MinimalObjectImpl const obj;
    ASSERT_EQ(to_string(obj), obj.to_string());
}

TEST(Object, ObjectWithCustomToStringOverrideReturnsExpectedString)
{
    std::string_view const expected = "some-injected-string";
    ASSERT_EQ(ObjectWithCustomToStringOverride{expected}.to_string(), expected);
    ASSERT_EQ(to_string(ObjectWithCustomToStringOverride{expected}), expected);
}

TEST(Object, ObjectWithGivenPropertiesWhenClonedReturnsSameNumProperties)
{
    auto const descriptions = osc::to_array<PropertyInfo>(
    {
        {"intprop", Variant{20}},
        {"floatprop", Variant{10.0f}},
        {"stringprop", Variant{"str"}},
    });
    ObjectWithGivenProperties const orig{descriptions};
    auto const clone = static_cast<Object const&>(orig).clone();
    ASSERT_EQ(orig.num_properties(), clone->num_properties());
}

TEST(Object, ObjectWithGivenPropertiesWhenClonedHasPropertyNamesInSameOrder)
{
    auto const descriptions = osc::to_array<PropertyInfo>(
    {
        {"intprop", Variant{20}},
        {"floatprop", Variant{10.0f}},
        {"stringprop", Variant{"str"}},
    });
    ObjectWithGivenProperties const orig{descriptions};
    auto const clone = static_cast<Object const&>(orig).clone();

    ASSERT_EQ(orig.num_properties(), clone->num_properties());
    for (size_t i = 0; i < orig.num_properties(); ++i)
    {
        ASSERT_EQ(orig.getPropertyName(i), clone->getPropertyName(i));
    }
}

TEST(Object, ObjectWithGivenPropertiesWhenClonedHasSamePropertyValues)
{
    auto const descriptions = osc::to_array<PropertyInfo>(
    {
        {"intprop", Variant{20}},
        {"floatprop", Variant{10.0f}},
        {"stringprop", Variant{"str"}},
    });
    auto const newIntValue = Variant{40};

    ObjectWithGivenProperties orig{descriptions};
    orig.set_property_value_or_throw("intprop", newIntValue);
    ASSERT_EQ(orig.property_value_or_throw("intprop"), newIntValue);

    auto const clone = static_cast<Object const&>(orig).clone();
    ASSERT_EQ(clone->property_value_or_throw("intprop"), newIntValue);
}

TEST(Object, ObjectWithGivenPropertiesWhenClonedHasSamePropertyNames)
{
    auto const descriptions = osc::to_array<PropertyInfo>(
    {
        {"intprop", Variant{20}},
        {"floatprop", Variant{10.0f}},
        {"stringprop", Variant{"str"}},
    });
    ObjectWithGivenProperties const orig{descriptions};
    auto const clone = static_cast<Object const&>(orig).clone();

    for (PropertyInfo const& description : descriptions)
    {
        ASSERT_TRUE(clone->property_index(description.name()));
    }
}

TEST(Object, ObjectWithGivenPropertiesWhenClonedHasSameDefaultValues)
{
    auto const descriptions = osc::to_array<PropertyInfo>(
    {
        {"intprop", Variant{20}},
        {"floatprop", Variant{10.0f}},
        {"stringprop", Variant{"str"}},
    });
    ObjectWithGivenProperties const orig{descriptions};
    auto const clone = static_cast<Object const&>(orig).clone();

    for (PropertyInfo const& description : descriptions)
    {
        ASSERT_EQ(clone->property_default_value_or_throw(description.name()), description.default_value());
    }
}

TEST(Object, GetNumPropertiesReturnsZeroWhenProvidedEmptySpan)
{
    std::array<PropertyInfo, 0> descriptions{};
    ObjectWithGivenProperties const a{descriptions};
    ASSERT_EQ(a.num_properties(), 0);
}

TEST(Object, GetNumPropertiesReturnsOneWhenProvidedOneProperty)
{
    auto const descriptions = osc::to_array<PropertyInfo>(
    {
        {"someprop", Variant{false}},
    });
    ObjectWithGivenProperties const a{descriptions};
    ASSERT_EQ(a.num_properties(), 1);
}

TEST(Object, GetNumPropertiesReturnsTwoWhenProvidedTwoProperties)
{
    auto const descriptions = osc::to_array<PropertyInfo>(
    {
        {"someprop", Variant{false}},
        {"somecolor", Variant{Color::red()}},
    });
    ObjectWithGivenProperties const a{descriptions};
    ASSERT_EQ(a.num_properties(), 2);
}

TEST(Object, GetNumPropertiesReturnsThreeWhenProvidedThreeProperties)
{
    auto const descriptions = osc::to_array<PropertyInfo>(
    {
        {"someprop", Variant{false}},
        {"somecolor", Variant{Color::red()}},
        {"somestring", Variant{"boring"}},
    });
    ObjectWithGivenProperties const a{descriptions};
    ASSERT_EQ(a.num_properties(), 3);
}

TEST(Object, GetNumPropertiesReturnsTwoWhenProvidedThreePropertyDescriptionsWithADuplicatedName)
{
    auto const descriptions = osc::to_array<PropertyInfo>(
    {
        {"a", Variant{false}},
        {"b", Variant{Color::red()}},
        {"a", Variant{"boring"}},
    });
    ObjectWithGivenProperties const a{descriptions};
    ASSERT_EQ(a.num_properties(), 2);
}

TEST(Object, GetNumPropertiesReturnsTheFirstProvidedPropertyDescriptionWhenGivenDuplicateNames)
{
    auto const descriptions = osc::to_array<PropertyInfo>(
    {
        {"a", Variant{false}},
        {"b", Variant{Color::red()}},
        {"a", Variant{"boring"}},
    });
    ObjectWithGivenProperties const a{descriptions};
    ASSERT_TRUE(a.property_value_or_throw("a").type() == VariantType::Bool);
}

TEST(Object, GetPropertyNameReturnsPropertiesInTheProvidedOrder)
{
    auto const descriptions = osc::to_array<PropertyInfo>(
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

    ASSERT_EQ(a.num_properties(), descriptions.size());
    for (size_t i = 0; i < descriptions.size(); ++i)
    {
        ASSERT_EQ(a.getPropertyName(i), descriptions.at(i).name());
    }
}

TEST(Object, GetPropertyIndexReturnsNulloptForInvalidName)
{
    auto const descriptions = osc::to_array<PropertyInfo>(
    {
        {"a", Variant{false}},
    });
    ObjectWithGivenProperties const a{descriptions};
    ASSERT_EQ(a.property_index("non-existent"), std::nullopt);
}

TEST(Object, GetPropertyIndexReturnsExpectedIndexForCorrectName)
{
    auto const descriptions = osc::to_array<PropertyInfo>(
    {
        {"a", Variant{false}},
        {"b", Variant{false}},
    });
    ObjectWithGivenProperties const a{descriptions};
    ASSERT_EQ(a.property_index("a"), 0);
    ASSERT_EQ(a.property_index("b"), 1);
}

TEST(Object, TryGetPropertyDefaultValueReturnsNullptrForInvalidName)
{
    auto const descriptions = osc::to_array<PropertyInfo>(
    {
        {"a", Variant{false}},
        {"b", Variant{false}},
    });
    ObjectWithGivenProperties const a{descriptions};
    ASSERT_EQ(a.property_default_value("non-existent"), nullptr);
}

TEST(Object, TryGetPropertyDefaultValueReturnsNonNullptrForCorrectName)
{
    auto const descriptions = osc::to_array<PropertyInfo>(
    {
        {"a", Variant{false}},
        {"b", Variant{false}},
    });
    ObjectWithGivenProperties const a{descriptions};
    ASSERT_NE(a.property_default_value("a"), nullptr);
    ASSERT_NE(a.property_default_value("b"), nullptr);
}

TEST(Object, TryGetPropertyDefaultValueReturnsNonNullptrToCorrectValueForCorrectName)
{
    auto const descriptions = osc::to_array<PropertyInfo>(
    {
        {"a", Variant{1337}},
        {"b", Variant{-1}},
    });
    ObjectWithGivenProperties const a{descriptions};
    ASSERT_NE(a.property_default_value("a"), nullptr);
    ASSERT_EQ(*a.property_default_value("a"), Variant{1337});
}

TEST(Object, GetPropertyDefaultValueThrowsForInvalidName)
{
    auto const descriptions = osc::to_array<PropertyInfo>(
    {
        {"a", Variant{1337}},
        {"b", Variant{-1}},
    });
    ObjectWithGivenProperties const a{descriptions};
    ASSERT_ANY_THROW({ a.property_default_value_or_throw("non-existent"); });
}

TEST(Object, GetPropertyDefaultValueDoesNotThrowForCorrectName)
{
    auto const descriptions = osc::to_array<PropertyInfo>(
    {
        {"a", Variant{1337}},
        {"b", Variant{-1}},
    });
    ObjectWithGivenProperties const a{descriptions};
    ASSERT_EQ(a.property_default_value_or_throw("a"), Variant{1337});
}

TEST(Object, TryGetPropertyValueReturnsNullptrForInvalidName)
{
    auto const descriptions = osc::to_array<PropertyInfo>(
    {
        {"a", Variant{false}},
        {"b", Variant{false}},
    });
    ObjectWithGivenProperties const a{descriptions};
    ASSERT_EQ(a.property_value("non-existent"), nullptr);
}

TEST(Object, TryGetPropertyValueReturnsNonNullptrForValidName)
{
    auto const descriptions = osc::to_array<PropertyInfo>(
    {
        {"a", Variant{false}},
        {"b", Variant{false}},
    });
    ObjectWithGivenProperties const a{descriptions};
    ASSERT_NE(a.property_value("a"), nullptr);
    ASSERT_NE(a.property_value("b"), nullptr);
}

TEST(Object, TryGetPropertyValueReturnsDefaultValueWhenValueHasNotBeenSet)
{
    auto const descriptions = osc::to_array<PropertyInfo>(
    {
        {"a", Variant{1337}},
        {"b", Variant{false}},
    });
    ObjectWithGivenProperties const a{descriptions};
    ASSERT_TRUE(a.property_value("a"));
    ASSERT_EQ(*a.property_value("a"), Variant{1337});
}

TEST(Object, TryGetPropertyValueReturnsNewValueAfterTheValueHasBeenSet)
{
    auto const oldValue = Variant{10};
    auto const newValue = Variant{50};
    auto const descriptions = osc::to_array<PropertyInfo>(
    {
        {"a", Variant{false}},
        {"b", oldValue},
    });

    ObjectWithGivenProperties a{descriptions};
    ASSERT_TRUE(a.property_value("b"));
    ASSERT_EQ(*a.property_value("b"), oldValue);
    a.set_property_value("b", newValue);
    ASSERT_TRUE(a.property_value("b"));
    ASSERT_EQ(*a.property_value("b"), newValue);
}

TEST(Object, GetPropertyValueThrowsIfGivenNonExistentPropertyName)
{
    auto const descriptions = osc::to_array<PropertyInfo>(
    {
        {"a", Variant{false}},
        {"b", Variant{false}},
    });
    ObjectWithGivenProperties const a{descriptions};
    ASSERT_ANY_THROW({ a.property_value_or_throw("a"); });
}

TEST(Object, GetPropertyValueReturnsValueIfGivenExistentPropertyName)
{
    auto const descriptions = osc::to_array<PropertyInfo>(
    {
        {"a", Variant{false}},
        {"b", Variant{"second"}},
    });
    ObjectWithGivenProperties const a{descriptions};
    ASSERT_EQ(a.property_value_or_throw("a"), Variant{"second"});
}

TEST(Object, GetPropertyValueReturnsNewValueAfterSettingValue)
{
    auto const oldValue = Variant{"old"};
    auto const newValue = Variant{"new"};
    auto const descriptions = osc::to_array<PropertyInfo>(
    {
        {"a", Variant{false}},
        {"b", oldValue},
    });

    ObjectWithGivenProperties a{descriptions};
    ASSERT_EQ(a.property_value_or_throw("b"), oldValue);
    a.set_property_value("b", newValue);
    ASSERT_EQ(a.property_value_or_throw("b"), newValue);
}

TEST(Object, TrySetPropertyValueReturnsFalseForNonExistentProperty)
{
    auto const descriptions = osc::to_array<PropertyInfo>(
    {
        {"a", Variant{"first"}},
        {"b", Variant{"second"}},
    });
    ObjectWithGivenProperties a{descriptions};
    ASSERT_FALSE(a.set_property_value("non-existent", Variant{"doesnt-matter"}));
}

TEST(Object, TrySetPropertyValueReturnsFalseIfVariantTypeMismatches)
{
    auto const descriptions = osc::to_array<PropertyInfo>(
    {
        {"a", Variant{"first"}},
        {"b", Variant{100}},
    });
    ObjectWithGivenProperties a{descriptions};
    ASSERT_FALSE(a.set_property_value("b", Variant{"not-an-int"}));
}

TEST(Object, TrySetPropertyValueReturnsTrueForCorrectPropertyNameAndType)
{
    auto const descriptions = osc::to_array<PropertyInfo>(
    {
        {"a", Variant{"old-string"}},
        {"b", Variant{100}},
    });
    ObjectWithGivenProperties a{descriptions};
    ASSERT_TRUE(a.set_property_value("a", Variant{"new-string"}));
}

TEST(Object, TrySetPropertyAfterReturningTrueMeansThatGetPropertyReturnsNewValue)
{
    auto const oldValue = Variant{"old-value"};
    auto const newValue = Variant{"new-value"};
    auto const descriptions = osc::to_array<PropertyInfo>(
    {
        {"a", oldValue},
        {"b", Variant{100}},
    });

    ObjectWithGivenProperties a{descriptions};
    ASSERT_EQ(a.property_value_or_throw("a"), oldValue);
    ASSERT_TRUE(a.set_property_value("a", newValue));
    ASSERT_EQ(a.property_value_or_throw("a"), newValue);
}

TEST(Object, TrySetPropertyAfterReturningTrueMakesTryGetPropertyvalueReturnNewValue)
{
    auto const oldValue = Variant{"old-value"};
    auto const newValue = Variant{"new-value"};
    auto const descriptions = osc::to_array<PropertyInfo>(
    {
        {"a", oldValue},
        {"b", Variant{100}},
    });

    ObjectWithGivenProperties a{descriptions};
    ASSERT_TRUE(a.property_value("a"));
    ASSERT_EQ(*a.property_value("a"), oldValue);
    ASSERT_TRUE(a.set_property_value("a", newValue));
    ASSERT_TRUE(a.property_value("a"));
    ASSERT_EQ(*a.property_value("a"), newValue);
}

TEST(Object, SetPropertyValueThrowsAnExceptionForNonExistentPropertyName)
{
    auto const descriptions = osc::to_array<PropertyInfo>(
    {
        {"a", Variant{"some-value"}},
        {"b", Variant{100}},
    });
    ObjectWithGivenProperties a{descriptions};

    ASSERT_ANY_THROW({ a.set_property_value_or_throw("doesnt-exist", Variant{"doesnt-matter"}); });
}

TEST(Object, SetPropertyValueThrowsIfGivenAMismatchedVariantType)
{
    auto const descriptions = osc::to_array<PropertyInfo>(
    {
        {"a", Variant{"some-value"}},
        {"b", Variant{100}},
    });
    ObjectWithGivenProperties a{descriptions};

    ASSERT_ANY_THROW({ a.set_property_value_or_throw("b", Variant{"not-a-number"}); });
}

TEST(Object, SetPropertyValueDoesntThrowIfGivenValidArguments)
{
    auto const descriptions = osc::to_array<PropertyInfo>(
    {
        {"a", Variant{"some-value"}},
        {"b", Variant{100}},
    });
    ObjectWithGivenProperties a{descriptions};

    ASSERT_NO_THROW({ a.set_property_value_or_throw("b", Variant{200}); });
}

TEST(Object, SetPropertyValueWithValidArgumentsMakesGetPropertyValueReturnNewValue)
{
    auto const oldValue = Variant{"old-value"};
    auto const newValue = Variant{"new-value"};
    auto const descriptions = osc::to_array<PropertyInfo>(
    {
        {"a", oldValue},
        {"b", Variant{100}},
    });

    ObjectWithGivenProperties a{descriptions};
    ASSERT_EQ(a.property_value_or_throw("a"), oldValue);
    ASSERT_NO_THROW({ a.set_property_value_or_throw("a", newValue); });
    ASSERT_EQ(a.property_value_or_throw("a"), newValue);
}

// TODO: with implCustomPropertyGetter:
//
// - property_value returns getter value if non-nullptr
// - property_value_or_throw returns getter value if non-nullptr
// - property_value returns "normal" value if nullptr
// - property_value_or_throw returns "normal" value if nullptr

*/
