#include <oscar/DOM/Class.h>

#include <gtest/gtest.h>
#include <oscar/DOM/PropertyInfo.h>
#include <oscar/Utils/StringName.h>
#include <oscar/Variant/Variant.h>

#include <algorithm>
#include <array>
#include <optional>
#include <ranges>
#include <span>
#include <vector>

using namespace osc;
namespace rgs = std::ranges;

namespace
{
    template<class T, size_t N, size_t M>
    auto concat(const std::array<T, N>& a1, const std::array<T, M>& a2) -> std::array<T, N+M>
    {
        std::array<T, N+M> rv;
        rgs::copy(a1, rv.begin());
        rgs::copy(a2, rv.begin() + N);
        return rv;
    }
}

TEST(Class, default_constructor_returns_class_name_of_object_no_parent_class_no_properties)
{
    Class c;
    ASSERT_EQ(c.name(), "Object");
    ASSERT_EQ(c.parent_class(), std::nullopt);
    ASSERT_TRUE(c.properties().empty());
}

TEST(Class, constructor_does_not_throw_for_simple_valid_arguments)
{
    ASSERT_NO_THROW({ Class("validclassname", Class{}, std::span<PropertyInfo>{}); });
}

TEST(Class, constructor_throws_if_given_invalid_class_name)
{
    const auto invalid_class_names = std::to_array({
        " leadingspace",
        "trailingSpace ",
        " spaces ",
        "5tartswithnumber",
        "-hyphenstart",
        "hyphen-mid",
        "hyphentrail-",
        "$omeothersymbol",
    });
    for (const auto& invalid_class_name : invalid_class_names) {
        ASSERT_ANY_THROW({ Class{invalid_class_name}; });
    }
}

TEST(Class, constructor_throws_if_given_duplicate_PropertyInfo)
{
    const auto properties = std::to_array({
        PropertyInfo{"duplicate", Variant{"shouldnt-matter"}},
        PropertyInfo{"b", Variant{"shouldnt-matter"}},
        PropertyInfo{"duplicate", Variant{"shouldnt-matter"}},
    });

    ASSERT_ANY_THROW({ Class("validclassname", Class{}, properties); });
}

TEST(Class, constructor_throws_if_given_PropertyInfo_that_is_duplicated_in_parent_class)
{
    const Class parent_class{
        "ParentClass",
        Class{},
        std::to_array({
            PropertyInfo{"parentprop", Variant{"shouldnt-matter"}},
        }),
    };

    ASSERT_ANY_THROW(
    {
        const Class child_class(
            "ChildClass",
            parent_class,
            std::to_array({
                PropertyInfo{"parentprop", Variant{"should-throw"}},
            })
        );
    });
}

TEST(Class, constructor_throws_if_given_PropertyInfo_that_is_duplicated_in_grandparent_class)
{
    const Class grandparent_class{
        "GrandparentClass",
        Class{},
        std::to_array({
            PropertyInfo{"grandparentProp", Variant{"shouldnt-matter"}},
        }),
    };

    const Class parent_class{
        "ParentClass",
        grandparent_class,
        std::to_array({
            PropertyInfo{"parentprop", Variant{"shouldnt-matter"}},
        }),
    };

    ASSERT_ANY_THROW(
    {
        const Class child_class(
            "ChildClass",
            parent_class,
            std::to_array({
                PropertyInfo{"grandparentProp", Variant{"should-throw"}},
            })
        );
    });
}

TEST(Class, name_returns_name_provided_via_constructor)
{
    const StringName class_name{"SomeClass"};
    ASSERT_EQ(Class{class_name}.name(), class_name);
}

TEST(Class, parent_class_returns_parent_class_provided_via_constructor)
{
    const Class parent_class{"ParentClass"};
    ASSERT_EQ(Class("SomeClass", parent_class).parent_class(), parent_class);
}

TEST(Class, properties_returns_property_list_provided_via_constructor)
{
    const auto properties_provided = std::to_array({
        PropertyInfo{"Prop1", Variant{true}},
        PropertyInfo{"Prop2", Variant{"false"}},
        PropertyInfo{"Prop3", Variant{7}},
        PropertyInfo{"Prop4", Variant{7.5f}},
    });

    const Class klass{"SomeClass", Class{}, properties_provided};
    ASSERT_TRUE(rgs::equal(klass.properties(), properties_provided));
}

TEST(Class, properties_returns_union_of_parent_properties_and_properties_provided_via_constructor)
{
    const auto base_properties = std::to_array({
        PropertyInfo{"Prop1", Variant{true}},
        PropertyInfo{"Prop2", Variant{"false"}},
        PropertyInfo{"Prop3", Variant{7}},
        PropertyInfo{"Prop4", Variant{7.5f}},
    });
    const Class base_class{"BaseClass", Class{}, base_properties};

    const auto derived_properties = std::to_array({
        PropertyInfo{"Prop5", Variant{11}},
        PropertyInfo{"Prop6", Variant{false}},
    });
    const Class derived_class{"DerivedClass", base_class, derived_properties};

    ASSERT_TRUE(rgs::equal(derived_class.properties(), concat(base_properties, derived_properties)));
}

TEST(Class, property_index_returns_indices_in_expected_order)
{
    const auto base_properties = std::to_array({
        PropertyInfo{"Prop1", Variant{true}},
        PropertyInfo{"Prop2", Variant{"false"}},
        PropertyInfo{"Prop3", Variant{7}},
        PropertyInfo{"Prop4", Variant{7.5f}},
    });
    const Class base_class{"BaseClass", Class{}, base_properties};

    const auto derived_properties = std::to_array({
        PropertyInfo{"Prop5", Variant{11}},
        PropertyInfo{"Prop6", Variant{false}},
    });
    const Class derived_class{"DerivedClass", base_class, derived_properties};

    const auto expected_property_order = concat(base_properties, derived_properties);
    for (size_t i = 0; i < expected_property_order.size(); ++i) {
        ASSERT_EQ(derived_class.property_index(expected_property_order[i].name()), i);
    }
}

TEST(Class, equality_returns_true_when_comparing_two_default_constructed_Class_instances)
{
    ASSERT_EQ(Class{}, Class{});
}

TEST(Class, equality_returns_true_when_comparing_two_Class_instances_with_the_same_name)
{
    const StringName class_name{"SomeClass"};
    ASSERT_EQ(Class{class_name}, Class{class_name});
}

TEST(Class, equality_returns_true_when_comparing_two_Class_instances_with_same_name_and_same_parent)
{
    const StringName class_name{"SomeClass"};
    const Class parent{"ParentClass"};
    ASSERT_EQ(Class(class_name, parent), Class(class_name, parent));
}

TEST(Class, equality_returns_true_when_comparing_two_Class_instances_with_same_name_and_same_parent_and_same_properties)
{
    const StringName class_name{"SomeClass"};
    const Class parent{"ParentClass"};
    const auto properties = std::to_array({
        PropertyInfo{"Prop1", Variant{"FirstProp"}},
        PropertyInfo{"Prop2", Variant{"SecondProp"}},
    });

    ASSERT_EQ(Class(class_name, parent, properties), Class(class_name, parent, properties));
}

TEST(Class, equality_returns_true_when_comparing_copied_class_instances)
{
    const StringName class_name{"SomeClass"};
    const Class parent{"ParentClass"};
    const auto properties = std::to_array({
        PropertyInfo{"Prop1", Variant{"FirstProp"}},
        PropertyInfo{"Prop2", Variant{"SecondProp"}},
    });

    const Class klass{class_name, parent, properties};
    const Class klass_copy = klass;  // NOLINT(performance-unnecessary-copy-initialization)

    ASSERT_EQ(klass_copy, klass);
}

TEST(Class, equality_returns_false_when_comparing_Class_instances_with_different_names)
{
    ASSERT_NE(Class("SomeName"), Class("SomeDifferentName"));
}

TEST(Class, equality_returns_false_when_comparing_Class_instances_with_same_name_but_different_parent)
{
    const StringName class_name{"SomeClass"};
    ASSERT_NE(Class(class_name, Class("FirstParent")), Class(class_name, Class("SecondParent")));
}

TEST(Class, equality_returns_false_when_comparing_Class_instances_with_same_name_and_same_parent_but_different_properties)
{
    const StringName class_name{"SomeClass"};
    const Class parent{"ParentClass"};
    const auto lhs_properties = std::to_array({
        PropertyInfo{"Prop1", Variant{"FirstProp"}},
        PropertyInfo{"Prop2", Variant{"SecondProp"}},
    });
    const auto rhs_properties = std::to_array({
        PropertyInfo{"Prop3", Variant{"ThirdProp"}},
        PropertyInfo{"Prop4", Variant{"FourthProp"}},
    });
    ASSERT_NE(Class(class_name, parent, lhs_properties), Class(class_name, parent, rhs_properties));
}
