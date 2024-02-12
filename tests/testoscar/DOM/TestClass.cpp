#include <oscar/DOM/Class.h>

#include <gtest/gtest.h>
#include <oscar/DOM/PropertyInfo.h>
#include <oscar/Utils/StringName.h>
#include <oscar/Variant/Variant.h>

#include <algorithm>
#include <array>
#include <optional>
#include <span>
#include <vector>

using namespace osc;

namespace
{
    template<class T, size_t N, size_t M>
    auto Concat(std::array<T, N> const& a1, std::array<T, M> const& a2) -> std::array<T, N+M>
    {
        std::array<T, N+M> rv;
        std::copy(a1.begin(), a1.end(), rv.begin());
        std::copy(a2.begin(), a2.end(), rv.begin() + N);
        return rv;
    }
}

TEST(Class, DefaultConstructorReturnsClassNameOfObjectNoParentClassNoProperties)
{
    Class c;
    ASSERT_EQ(c.getName(), "Object");
    ASSERT_EQ(c.getParentClass(), std::nullopt);
    ASSERT_TRUE(c.getPropertyList().empty());
}

TEST(Class, ConstructorDoesNotThrowForSimpleValidArguments)
{
    ASSERT_NO_THROW({ Class("validclassname", Class{}, std::span<PropertyInfo>{}); });
}

TEST(Class, ConstructorThrowsIfGivenInvalidClassName)
{
    auto const invalidNames = std::to_array(
    {
        " leadingspace",
        "trailingSpace ",
        " spaces ",
        "5tartswithnumber",
        "-hyphenstart",
        "hyphen-mid",
        "hyphentrail-",
        "$omeothersymbol",
    });
    for (auto const& invalidName : invalidNames)
    {
        ASSERT_ANY_THROW({ Class{invalidName}; });
    }
}

TEST(Class, ConstructorThrowsIfGivenDuplicatePropertyInfo)
{
    auto const infos = std::to_array<PropertyInfo>(
    {
        {"duplicate", Variant{"shouldnt-matter"}},
        {"b", Variant{"shouldnt-matter"}},
        {"duplicate", Variant{"shouldnt-matter"}},
    });

    ASSERT_ANY_THROW({ Class("validclassname", Class{}, infos); });
}

TEST(Class, ConstructorThrowsIfGivenPropertyInfoThatIsDuplicatedInDirectParentClass)
{
    Class const parentClass
    {
        "ParentClass",
        Class{},
        std::to_array<PropertyInfo>(
        {
            {"parentprop", Variant{"shouldnt-matter"}},
        }),
    };

    ASSERT_ANY_THROW(
    {
        Class const childClass
        (
            "ChildClass",
            parentClass,
            std::to_array<PropertyInfo>(
            {
                {"parentprop", Variant{"should-throw"}},
            })
        );
    });
}

TEST(Class, ConstructorThrowsIfGivenPropertyInfoThatIsDuplicatedInGrandparent)
{
    Class const grandparentClass
    {
        "GrandparentClass",
        Class{},
        std::to_array<PropertyInfo>(
        {
            {"grandparentProp", Variant{"shouldnt-matter"}},
        }),
    };

    Class const parentClass
    {
        "ParentClass",
        grandparentClass,
        std::to_array<PropertyInfo>(
        {
            {"parentprop", Variant{"shouldnt-matter"}},
        }),
    };

    ASSERT_ANY_THROW(
    {
        Class const childClass
        (
            "ChildClass",
            parentClass,
            std::to_array<PropertyInfo>(
            {
                {"grandparentProp", Variant{"should-throw"}},
            })
        );
    });
}

TEST(Class, GetNameReturnsNameProvidedViaConstructor)
{
    StringName const className{"SomeClass"};
    ASSERT_EQ(Class{className}.getName(), className);
}

TEST(Class, GetParentClassReturnsParentClassProvidedViaConstructor)
{
    Class const parentClass{"ParentClass"};
    ASSERT_EQ(Class("SomeClass", parentClass).getParentClass(), parentClass);
}

TEST(Class, GetPropertyListReturnsProvidedPropertyListForAClassWithNoParents)
{
    auto const propList = std::to_array<PropertyInfo>(
    {
        {"Prop1", Variant{true}},
        {"Prop2", Variant{"false"}},
        {"Prop3", Variant{7}},
        {"Prop4", Variant{7.5f}},
    });

    Class const someClass{"SomeClass", Class{}, propList};
    std::span<PropertyInfo const> const returnedList = someClass.getPropertyList();

    ASSERT_TRUE(std::equal(propList.begin(), propList.end(), returnedList.begin(), returnedList.end()));
}

TEST(Class, GetPropertyListReturnsBaseClassPropertiesFollowedByDerivedClassProperties)
{
    auto const basePropList = std::to_array<PropertyInfo>(
    {
        {"Prop1", Variant{true}},
        {"Prop2", Variant{"false"}},
        {"Prop3", Variant{7}},
        {"Prop4", Variant{7.5f}},
    });
    Class const baseClass{"BaseClass", Class{}, basePropList};

    auto const derivedPropList = std::to_array<PropertyInfo>(
    {
        {"Prop5", Variant{11}},
        {"Prop6", Variant{false}},
    });
    Class const derivedClass{"DerivedClass", baseClass, derivedPropList};

    auto const returnedPropList = derivedClass.getPropertyList();
    auto const expectedPropList = Concat(basePropList, derivedPropList);

    ASSERT_TRUE(std::equal(returnedPropList.begin(), returnedPropList.end(), expectedPropList.begin(), expectedPropList.end()));
}

TEST(Class, GetPropertyIndexReturnsExpectedIndices)
{
    auto const baseProps = std::to_array<PropertyInfo>(
    {
        {"Prop1", Variant{true}},
        {"Prop2", Variant{"false"}},
        {"Prop3", Variant{7}},
        {"Prop4", Variant{7.5f}},
    });
    Class const baseClass{"BaseClass", Class{}, baseProps};

    auto const derivedProps = std::to_array<PropertyInfo>(
    {
        {"Prop5", Variant{11}},
        {"Prop6", Variant{false}},
    });
    Class const derivedClass{"DerivedClass", baseClass, derivedProps};

    for (size_t i = 0; i < baseProps.size(); ++i)
    {
        ASSERT_EQ(derivedClass.getPropertyIndex(baseProps[i].getName()), i);
    }
    for (size_t i = 0; i < derivedProps.size(); ++i)
    {
        ASSERT_EQ(derivedClass.getPropertyIndex(derivedProps[i].getName()), baseProps.size() + i);
    }
}

TEST(Class, EqualityReturnsTrueForTwoDefaultConstructedClasses)
{
    ASSERT_EQ(Class{}, Class{});
}

TEST(Class, EqualityReturnsTrueForTwoClassesWithTheSameName)
{
    StringName const className{"SomeClass"};
    ASSERT_EQ(Class{className}, Class{className});
}

TEST(Class, EqualityReturnsTrueForTwoClassesWithSameNameAndSameParent)
{
    StringName const className{"SomeClass"};
    Class const parent{"ParentClass"};
    ASSERT_EQ(Class(className, parent), Class(className, parent));
}

TEST(Class, EqualityReturnsTrueForTwoClassesWithSameNameAndParentAndProperties)
{
    StringName const className{"SomeClass"};
    Class const parent{"ParentClass"};
    auto const props = std::to_array<PropertyInfo>(
    {
        {"Prop1", Variant{"FirstProp"}},
        {"Prop2", Variant{"SecondProp"}},
    });

    ASSERT_EQ(Class(className, parent, props), Class(className, parent, props));
}

TEST(Class, EqualityReutrnsTrueForCopiedClass)
{
    StringName const className{"SomeClass"};
    Class const parent{"ParentClass"};
    auto const props = std::to_array<PropertyInfo>(
    {
        {"Prop1", Variant{"FirstProp"}},
        {"Prop2", Variant{"SecondProp"}},
    });

    Class const original{className, parent, props};
    Class const copy = original;  // NOLINT(performance-unnecessary-copy-initialization)

    ASSERT_EQ(copy, original);
}

TEST(Class, EqualityReturnsFalseIfNamesDiffer)
{
    ASSERT_NE(Class("SomeName"), Class("SomeDifferentName"));
}

TEST(Class, EqualityReturnsFalseIfNamesSameButParentDiffers)
{
    StringName const className{"SomeClass"};
    ASSERT_NE(Class(className, Class("FirstParent")), Class(className, Class("SecondParent")));
}

TEST(Class, EqualityReturnsFalseIfNamesAndParentClassSameButPropListDiffers)
{
    StringName const className{"SomeClass"};
    Class const parent{"ParentClass"};
    auto const firstProps = std::to_array<PropertyInfo>(
    {
        {"Prop1", Variant{"FirstProp"}},
        {"Prop2", Variant{"SecondProp"}},
    });
    auto const secondProps = std::to_array<PropertyInfo>(
    {
        {"Prop3", Variant{"ThirdProp"}},
        {"Prop4", Variant{"FourthProp"}},
    });
    ASSERT_NE(Class(className, parent, firstProps), Class(className, parent, secondProps));
}
