#include "Vec3.h"

#include <gtest/gtest.h>

#include <concepts>
#include <ranges>
#include <type_traits>

using namespace osc;

TEST(Vec3, with_element_works_as_expected)
{
    ASSERT_EQ(Vec3{}.with_element(0, 2.0f), Vec3(2.0f, 0.0f, 0.0f));
    ASSERT_EQ(Vec3(1.0f).with_element(0, 3.0f), Vec3(3.0f, 1.0f, 1.0f));
    ASSERT_EQ(Vec3{}.with_element(1, 3.0f), Vec3(0.0f, 3.0f, 0.0f));
    ASSERT_EQ(Vec3{}.with_element(2, 3.0f), Vec3(0.0f, 0.0f, 3.0f));
}

TEST(Vec3, can_be_used_to_construct_a_span_of_floats)
{
    static_assert(std::ranges::contiguous_range<Vec3> and std::ranges::sized_range<Vec3>);
    static_assert(std::constructible_from<std::span<const float>, const Vec3&>);
    static_assert(not std::constructible_from<std::span<float>, const Vec3&>);
    static_assert(std::constructible_from<std::span<float>, Vec3&>);
}

TEST(Vec3, can_be_used_as_arg_to_sized_span_func)
{
    const auto f = []([[maybe_unused]] std::span<float, 3>) {};
    static_assert(std::invocable<decltype(f), Vec3>);
}
