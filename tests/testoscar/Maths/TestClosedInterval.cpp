#include <oscar/Maths/ClosedInterval.h>

#include <gtest/gtest.h>
#include <oscar/Maths/CommonFunctions.h>

#include <chrono>

using namespace osc;

TEST(ClosedInterval, default_constructor_value_initializes)
{
    ClosedInterval<float> r;
    ASSERT_EQ(r.lower, float{});
    ASSERT_EQ(r.upper, float{});
}

TEST(ClosedInterval, can_use_structured_bindings_to_get_lower_and_upper)
{
    const auto [lower, upper] = ClosedInterval<int>{1, 3};
    ASSERT_EQ(lower, 1);
    ASSERT_EQ(upper, 3);
}

TEST(ClosedInterval, CanConstructForInts)
{
    [[maybe_unused]] ClosedInterval<int> r{0, 1}; // shouldn't throw etc
}

TEST(ClosedInterval, ReversingOrderIsAllowed)
{
    [[maybe_unused]] ClosedInterval<int> r{1, 0};
}

TEST(ClosedInterval, TimestampsAreAllowed)
{
    using TP = std::chrono::system_clock::time_point;
    [[maybe_unused]] ClosedInterval<TP> r{TP{}, TP{} + std::chrono::seconds{1}};
}

TEST(ClosedInterval, normalized_interpolant_at_returns_zero_if_equal_to_lower)
{
    static_assert(ClosedInterval{-3.0f, 7.0f}.normalized_interpolant_at(-3.0f) == 0.0f);
}

TEST(ClosedInterval, normalized_interpolant_at_returns_1_if_equal_to_upper)
{
    static_assert(ClosedInterval{-3.0f, 7.0f}.normalized_interpolant_at(7.0f) == 1.0f);
}

TEST(ClosedInterval, normalized_interpolant_at_returns_0_for_any_finite_input_if_lower_equals_upper)
{
    // note: this matches `std::lerp`'s inverse behavior
    for (float v : {-5.0f, 0.0f, 1.0f, 7.0f}) {
        ASSERT_EQ(ClosedInterval(1.0f, 1.0f).normalized_interpolant_at(v), 0.0f);
    }
}

TEST(ClosedInterval, step_size_returns_expected_answers)
{
    static_assert(ClosedInterval{0.0f, 0.0f}.step_size(0) == 0.0f);
    static_assert(ClosedInterval{0.0f, 0.0f}.step_size(1) == 0.0f);

    static_assert(ClosedInterval{0.0f, 1.0f}.step_size(0) == 1.0f);
    static_assert(ClosedInterval{0.0f, 1.0f}.step_size(1) == 1.0f);
    static_assert(ClosedInterval{0.0f, 1.0f}.step_size(2) == 1.0f);
    static_assert(ClosedInterval{0.0f, 1.0f}.step_size(3) == 0.5f);
}

TEST(ClosedInterval, contains_works_as_expected)
{
    static_assert(ClosedInterval<float>{0.0f, 0.0f}.contains(0.0f));
    static_assert(ClosedInterval<float>{-1.0f, 1.0f}.contains(0.0f));
    static_assert(not ClosedInterval<float>{0.0f, 1.0f}.contains(-0.1f));
    static_assert(not ClosedInterval<float>{0.0f, 1.0f}.contains(1.1f));
}

TEST(ClosedInterval, contains_also_works_for_ints)
{
    static_assert(ClosedInterval<int>{0, 0}.contains(0));
    static_assert(ClosedInterval<int>{-1, 1}.contains(0));
    static_assert(not ClosedInterval<int>{0, 1}.contains(-1));
    static_assert(not ClosedInterval<int>{0, 1}.contains(2));
}

TEST(ClosedInterval, unit_interval_works_for_floats)
{
    static_assert(unit_interval<float>() == ClosedInterval{0.0f, 1.0f});
}

TEST(ClosedInterval, unit_interval_works_for_doubles)
{
    static_assert(unit_interval<double>() == ClosedInterval{0.0, 1.0});
}

TEST(ClosedInterval, bounding_interval_of_for_single_entry_returns_expected_interval)
{
    static_assert(bounding_interval_of(7.0f) == ClosedInterval{7.0f, 7.0f});
}

TEST(ClosedInterval, bounding_interval_of_for_interval_and_single_entry_returns_expected_results)
{
    static_assert(bounding_interval_of(ClosedInterval{0.0f, 0.5f}, 1.0f) == ClosedInterval{0.0f, 1.0f});
    static_assert(bounding_interval_of(ClosedInterval{0.0f, 0.5f}, -1.0f) == ClosedInterval{-1.0f, 0.5f});
}

TEST(ClosedInterval, bounding_interval_of_for_optional_interval_and_single_value_returns_expected_results)
{
    static_assert(bounding_interval_of<float>(std::nullopt, 1.0f) == ClosedInterval{1.0f, 1.0f});
    static_assert(bounding_interval_of<float>(std::optional<ClosedInterval<float>>{ClosedInterval<float>{0.0f, 1.0f}}, 1.5f) == ClosedInterval{0.0f, 1.5f});
}
