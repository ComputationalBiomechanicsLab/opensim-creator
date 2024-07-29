#include <oscar/Maths/ClosedInterval.h>

#include <gtest/gtest.h>
#include <oscar/Maths/CommonFunctions.h>

#include <chrono>

using namespace osc;

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

TEST(ClosedInterval, step_size_returns_expected_answers)
{
    static_assert(ClosedInterval{0.0f, 0.0f}.step_size(0) == 0.0f);
    static_assert(ClosedInterval{0.0f, 0.0f}.step_size(1) == 0.0f);

    static_assert(ClosedInterval{0.0f, 1.0f}.step_size(0) == 1.0f);
    static_assert(ClosedInterval{0.0f, 1.0f}.step_size(1) == 1.0f);
    static_assert(ClosedInterval{0.0f, 1.0f}.step_size(2) == 1.0f);
    static_assert(ClosedInterval{0.0f, 1.0f}.step_size(3) == 0.5f);
}
