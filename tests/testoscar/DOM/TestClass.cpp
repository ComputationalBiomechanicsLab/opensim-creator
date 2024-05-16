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
    auto concat(const std::array<T, N>& a1, const std::array<T, M>& a2) -> std::array<T, N+M>
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
    ASSERT_EQ(c.name(), "Object");
    ASSERT_EQ(c.parent_class(), std::nullopt);
    ASSERT_TRUE(c.properties().empty());
}

TEST(Class, ConstructorDoesNotThrowForSimpleValidArguments)
{
    ASSERT_NO_THROW({ Class("validclassname", Class{}, std::span<PropertyInfo>{}); });
}

TEST(Class, ConstructorThrowsIfGivenInvalidClassName)
{
    const auto invalid_names = std::to_array({
        " leadingspace",
        "trailingSpace ",
        " spaces ",
        "5tartswithnumber",
        "-hyphenstart",
        "hyphen-mid",
        "hyphentrail-",
        "$omeothersymbol",
    });
    for (const auto& invalid_name : invalid_names) {
        ASSERT_ANY_THROW({ Class{invalid_name}; });
    }
}

TEST(Class, ConstructorThrowsIfGivenDuplicatePropertyInfo)
{
    const auto infos = std::to_array<PropertyInfo>({
        {"duplicate", Variant{"shouldnt-matter"}},
        {"b", Variant{"shouldnt-matter"}},
        {"duplicate", Variant{"shouldnt-matter"}},
    });

    ASSERT_ANY_THROW({ Class("validclassname", Class{}, infos); });
}

TEST(Class, ConstructorThrowsIfGivenPropertyInfoThatIsDuplicatedInDirectParentClass)
{
    const Class parent_class{
        "ParentClass",
        Class{},
        std::to_array<PropertyInfo>({
            {"parentprop", Variant{"shouldnt-matter"}},
        }),
    };

    ASSERT_ANY_THROW(
    {
        const Class child_class
        (
            "ChildClass",
            parent_class,
            std::to_array<PropertyInfo>({
                {"parentprop", Variant{"should-throw"}},
            })
        );
    });
}

TEST(Class, ConstructorThrowsIfGivenPropertyInfoThatIsDuplicatedInGrandparent)
{
    const Class grandparent_class
    {
        "GrandparentClass",
        Class{},
        std::to_array<PropertyInfo>({
            {"grandparentProp", Variant{"shouldnt-matter"}},
        }),
    };

    const Class parent_class
    {
        "ParentClass",
        grandparent_class,
        std::to_array<PropertyInfo>({
            {"parentprop", Variant{"shouldnt-matter"}},
        }),
    };

    ASSERT_ANY_THROW(
    {
        const Class child_class
        (
            "ChildClass",
            parent_class,
            std::to_array<PropertyInfo>({
                {"grandparentProp", Variant{"should-throw"}},
            })
        );
    });
}

TEST(Class, name_ReturnsNameProvidedViaConstructor)
{
    const StringName class_name{"SomeClass"};
    ASSERT_EQ(Class{class_name}.name(), class_name);
}

TEST(Class, parent_class_ReturnsParentClassProvidedViaConstructor)
{
    const Class parent_class{"ParentClass"};
    ASSERT_EQ(Class("SomeClass", parent_class).parent_class(), parent_class);
}

TEST(Class, properties_ReturnsProvidedPropertyListForAClassWithNoParents)
{
    const auto prop_list = std::to_array<PropertyInfo>({
        {"Prop1", Variant{true}},
        {"Prop2", Variant{"false"}},
        {"Prop3", Variant{7}},
        {"Prop4", Variant{7.5f}},
    });

    const Class some_class{"SomeClass", Class{}, prop_list};
    const std::span<const PropertyInfo> returned_list = some_class.properties();

    ASSERT_TRUE(std::equal(prop_list.begin(), prop_list.end(), returned_list.begin(), returned_list.end()));
}

TEST(Class, properties_ReturnsBaseClassPropertiesFollowedByDerivedClassProperties)
{
    const auto base_prop_list = std::to_array<PropertyInfo>({
        {"Prop1", Variant{true}},
        {"Prop2", Variant{"false"}},
        {"Prop3", Variant{7}},
        {"Prop4", Variant{7.5f}},
    });
    const Class base_class{"BaseClass", Class{}, base_prop_list};

    const auto derived_prop_list = std::to_array<PropertyInfo>({
        {"Prop5", Variant{11}},
        {"Prop6", Variant{false}},
    });
    const Class derived_class{"DerivedClass", base_class, derived_prop_list};

    const auto returned_prop_list = derived_class.properties();
    const auto expected_prop_list = concat(base_prop_list, derived_prop_list);

    ASSERT_TRUE(std::equal(returned_prop_list.begin(), returned_prop_list.end(), expected_prop_list.begin(), expected_prop_list.end()));
}

TEST(Class, property_index_ReturnsExpectedIndices)
{
    const auto base_props = std::to_array<PropertyInfo>({
        {"Prop1", Variant{true}},
        {"Prop2", Variant{"false"}},
        {"Prop3", Variant{7}},
        {"Prop4", Variant{7.5f}},
    });
    const Class base_class{"BaseClass", Class{}, base_props};

    const auto derived_props = std::to_array<PropertyInfo>({
        {"Prop5", Variant{11}},
        {"Prop6", Variant{false}},
    });
    const Class derived_class{"DerivedClass", base_class, derived_props};

    for (size_t i = 0; i < base_props.size(); ++i) {
        ASSERT_EQ(derived_class.property_index(base_props[i].name()), i);
    }
    for (size_t i = 0; i < derived_props.size(); ++i) {
        ASSERT_EQ(derived_class.property_index(derived_props[i].name()), base_props.size() + i);
    }
}

TEST(Class, EqualityReturnsTrueForTwoDefaultConstructedClasses)
{
    ASSERT_EQ(Class{}, Class{});
}

TEST(Class, EqualityReturnsTrueForTwoClassesWithTheSameName)
{
    const StringName class_name{"SomeClass"};
    ASSERT_EQ(Class{class_name}, Class{class_name});
}

TEST(Class, EqualityReturnsTrueForTwoClassesWithSameNameAndSameParent)
{
    const StringName class_name{"SomeClass"};
    const Class parent{"ParentClass"};
    ASSERT_EQ(Class(class_name, parent), Class(class_name, parent));
}

TEST(Class, EqualityReturnsTrueForTwoClassesWithSameNameAndParentAndProperties)
{
    const StringName class_name{"SomeClass"};
    const Class parent{"ParentClass"};
    const auto props = std::to_array<PropertyInfo>({
        {"Prop1", Variant{"FirstProp"}},
        {"Prop2", Variant{"SecondProp"}},
    });

    ASSERT_EQ(Class(class_name, parent, props), Class(class_name, parent, props));
}

TEST(Class, EqualityReutrnsTrueForCopiedClass)
{
    const StringName class_name{"SomeClass"};
    const Class parent{"ParentClass"};
    const auto props = std::to_array<PropertyInfo>({
        {"Prop1", Variant{"FirstProp"}},
        {"Prop2", Variant{"SecondProp"}},
    });

    const Class original{class_name, parent, props};
    const Class copy = original;  // NOLINT(performance-unnecessary-copy-initialization)

    ASSERT_EQ(copy, original);
}

TEST(Class, EqualityReturnsFalseIfNamesDiffer)
{
    ASSERT_NE(Class("SomeName"), Class("SomeDifferentName"));
}

TEST(Class, EqualityReturnsFalseIfNamesSameButParentDiffers)
{
    const StringName class_name{"SomeClass"};
    ASSERT_NE(Class(class_name, Class("FirstParent")), Class(class_name, Class("SecondParent")));
}

TEST(Class, EqualityReturnsFalseIfNamesAndParentClassSameButPropListDiffers)
{
    const StringName class_name{"SomeClass"};
    const Class parent{"ParentClass"};
    const auto first_props = std::to_array<PropertyInfo>({
        {"Prop1", Variant{"FirstProp"}},
        {"Prop2", Variant{"SecondProp"}},
    });
    const auto second_props = std::to_array<PropertyInfo>({
        {"Prop3", Variant{"ThirdProp"}},
        {"Prop4", Variant{"FourthProp"}},
    });
    ASSERT_NE(Class(class_name, parent, first_props), Class(class_name, parent, second_props));
}
