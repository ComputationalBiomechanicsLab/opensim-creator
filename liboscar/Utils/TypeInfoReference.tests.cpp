#include "TypeInfoReference.h"

#include <gtest/gtest.h>

#include <concepts>
#include <functional>
#include <set>
#include <type_traits>

using namespace osc;

TEST(TypeInfoReference, can_construct_from_typeid_expression)
{
    static_assert(std::is_constructible_v<TypeInfoReference, decltype(typeid(int))>);
}

TEST(TypeInfoReference, can_be_implicitly_converted_from_typeid)
{
    [[maybe_unused]] const TypeInfoReference info = typeid(short);  // ensure this compiles
}

TEST(TypeInfoReference, get_returns_reference_to_the_type_info_used_to_construct_instance)
{
    ASSERT_EQ(TypeInfoReference{typeid(char)}.get(), typeid(char));
}

TEST(TypeInfoReference, operator_equals_works_in_standard_case)
{
    ASSERT_EQ(TypeInfoReference{typeid(bool)}, TypeInfoReference{typeid(bool)});
}

TEST(TypeInfoReference, operator_equals_can_also_compare_with_std_type_info)
{
    ASSERT_EQ(TypeInfoReference{typeid(void*)}, typeid(void*));
    ASSERT_EQ(typeid(void*), TypeInfoReference{typeid(void*)});
}

TEST(TypeInfoReference, operator_less_than_calls_before)
{
    const TypeInfoReference a{typeid(int*)};
    const TypeInfoReference b{typeid(char)};
    ASSERT_EQ(a < b, typeid(int*).before(typeid(char)));
    ASSERT_EQ(b < a, typeid(char).before(typeid(int*)));
    ASSERT_NE(a < b, b < a);
}

TEST(TypeInfoReference, is_hashable)
{
    const auto h = std::hash<TypeInfoReference>{}(TypeInfoReference{typeid(double)});
    ASSERT_NE(h, 0) << "hopefully, the hash of a `double` typeid is non-zero (it could be zero, though - remove this if it is ;))";
}

TEST(TypeInfoReference, can_be_used_in_a_set)
{
    // this just confirms that it works as expected
    std::set<TypeInfoReference> s;
    s.emplace(typeid(double));
    ASSERT_EQ(s.size(), 1);
    s.emplace(typeid(int));
    ASSERT_EQ(s.size(), 2);
    s.emplace(typeid(short));
    ASSERT_EQ(s.size(), 3);
    s.emplace(typeid(int));
    ASSERT_EQ(s.size(), 3);
    s.emplace(typeid(short));
    ASSERT_EQ(s.size(), 3);
    s.emplace(typeid(void*));
    ASSERT_EQ(s.size(), 4);
}
