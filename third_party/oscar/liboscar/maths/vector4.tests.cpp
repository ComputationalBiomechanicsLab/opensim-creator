#include "vector4.h"

#include <gtest/gtest.h>

#include <concepts>
#include <ranges>
#include <type_traits>

using namespace osc;

TEST(Vector4, with_element_works_as_expected)
{
    ASSERT_EQ(Vector4{}.with_element(0, 2.0f), Vector4(2.0f, 0.0f, 0.0f, 0.0f));
    ASSERT_EQ(Vector4(1.0f).with_element(0, 3.0f), Vector4(3.0f, 1.0f, 1.0f, 1.0f));
    ASSERT_EQ(Vector4{}.with_element(1, 3.0f), Vector4(0.0f, 3.0f, 0.0f, 0.0f));
    ASSERT_EQ(Vector4{}.with_element(2, 3.0f), Vector4(0.0f, 0.0f, 3.0f, 0.0f));
    ASSERT_EQ(Vector4{}.with_element(3, 3.0f), Vector4(0.0f, 0.0f, 0.0f, 3.0f));
}

TEST(Vector4, can_be_used_to_construct_a_span_of_floats)
{
    static_assert(std::ranges::contiguous_range<Vector4> and std::ranges::sized_range<Vector4>);
    static_assert(std::constructible_from<std::span<const float>, const Vector4&>);
    static_assert(not std::constructible_from<std::span<float>, const Vector4&>);
    static_assert(std::constructible_from<std::span<float>, Vector4&>);
}

TEST(Vector4, can_be_used_as_arg_to_sized_span_func)
{
    const auto f = []([[maybe_unused]] std::span<float, 4>) {};
    static_assert(std::invocable<decltype(f), Vector4>);
}
