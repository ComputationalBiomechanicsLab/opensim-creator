#include "Vector2.h"

#include <gtest/gtest.h>

#include <concepts>
#include <ranges>
#include <type_traits>

using namespace osc;

TEST(Vector2, with_element_works_as_expected)
{
    ASSERT_EQ(Vector2{}.with_element(0, 2.0f), Vector2(2.0f, 0.0f));
    ASSERT_EQ(Vector2(1.0f).with_element(0, 3.0f), Vector2(3.0f, 1.0f));
    ASSERT_EQ(Vector2{}.with_element(1, 3.0f), Vector2(0.0f, 3.0f));
}

TEST(Vector2, can_be_used_to_construct_a_span_of_floats)
{
    static_assert(std::ranges::contiguous_range<Vector2> and std::ranges::sized_range<Vector2>);
    static_assert(std::constructible_from<std::span<const float>, const Vector2&>);
    static_assert(not std::constructible_from<std::span<float>, const Vector2&>);
    static_assert(std::constructible_from<std::span<float>, Vector2&>);
}

TEST(Vector2, can_be_used_as_arg_to_sized_span_func)
{
    const auto f = []([[maybe_unused]] std::span<float, 2>) {};
    static_assert(std::invocable<decltype(f), Vector2>);
}
