#include <oscar/Maths/ClosedInterval.h>

#include <gtest/gtest.h>

#include <chrono>

using namespace osc;

TEST(ClosedInterval, CanConstructForInts)
{
    ClosedInterval<int>{0, 1}; // shouldn't throw etc
}

TEST(ClosedInterval, ReversingOrderIsAllowed)
{
    ClosedInterval<int>{1, 0};
}

TEST(ClosedInterval, TimestampsAreAllowed)
{
    using TP = std::chrono::system_clock::time_point;
    ClosedInterval<TP>{TP{}, TP{} + std::chrono::seconds{1}};
}
