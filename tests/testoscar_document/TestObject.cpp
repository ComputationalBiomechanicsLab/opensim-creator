#include <oscar_document/Object.hpp>

#include <oscar_document/PropertyDescription.hpp>

#include <gtest/gtest.h>
#include <nonstd/span.hpp>

#include <memory>
#include <random>
#include <string>
#include <string_view>

/*

using osc::doc::Object;
using osc::doc::PropertyDescription;
using osc::doc::Variant;

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

TEST(Object, MinimalObjectImplCanConstruct)
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
    ASSERT_ANY_THROW({ MinimalObjectImpl{}.getPropertyName(0); });
}

TEST(Object, MinimalObjectImplGetPropertyTypeIsBoundsChecked)
{
    ASSERT_ANY_THROW({ MinimalObjectImpl{}.getPropertyType(0); });
}

TEST(Object, MinimalObjectImplGetPropertyDefaultValueIsBoundsChecked)
{
    ASSERT_ANY_THROW({ MinimalObjectImpl{}.getPropertyDefaultValue(0); });
}

TEST(Object, MinimalObjectImplGetPropertyValueIsBoundsChecked)
{
    ASSERT_ANY_THROW({ MinimalObjectImpl{}.getPropertyValue(0); });
}

TEST(Object, MinimalObjectImplSetPropertyValueIsBoundsChecked)
{
    ASSERT_ANY_THROW({ MinimalObjectImpl{}.setPropertyValue(0, Variant{false}); });
}

TEST(Object, MinimalObjectImplGetPropertyIndexByNameReturnsFalsey)
{
    ASSERT_FALSE(MinimalObjectImpl().getPropertyIndexByName("non-existent"));
}

TEST(Object, MinimalObjectImplTryGetPropertyValueByNameReturnsFalsey)
{
    ASSERT_FALSE(MinimalObjectImpl().tryGetPropertyValueByName("non-existent"));
}

TEST(Object, MinimalObjectImplTrySetPropertyValueByNameReturnsFalsey)
{
    ASSERT_FALSE(MinimalObjectImpl().trySetPropertyValueByName("non-existent", Variant{true}));
}

TEST(Object, MinimalObjectImplGetPropertyByNameThrows)
{
    ASSERT_ANY_THROW({ MinimalObjectImpl{}.getPropertyValueByName("non-existent"); });
}

TEST(Object, MinimalObjectImplSetPropertyValueByNameThrows)
{
    ASSERT_ANY_THROW({ MinimalObjectImpl{}.setPropertyValueByName("non-existent", Variant{false}); });
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

// todo: object with properties

*/
