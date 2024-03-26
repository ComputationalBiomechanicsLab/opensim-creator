#include <oscar/Maths/ClosedInterval.h>

#include <gtest/gtest.h>

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
