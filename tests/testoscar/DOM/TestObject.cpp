#include <oscar/DOM/Object.hpp>

#include <oscar/DOM/PropertyDescription.hpp>

#include <gtest/gtest.h>
#include <nonstd/span.hpp>

#include <memory>
#include <random>
#include <string>
#include <string_view>

/*
using osc::Object;
using osc::PropertyDescription;
using osc::Variant;

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

// todo: object with properties

*/
