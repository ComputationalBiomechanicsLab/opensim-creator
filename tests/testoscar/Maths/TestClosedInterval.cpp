#include <oscar/Maths/ClosedInterval.h>

#include <gtest/gtest.h>

#include <chrono>

using namespace osc;

TEST(ClosedInterval, CanConstructForInts)
{
    ASSERT_NO_THROW({ ClosedInterval<int>(0, 1); });
}

TEST(ClosedInterval, ReversingOrderIsAllowed)
{
    ASSERT_NO_THROW({ ClosedInterval<int>(1, 0); });
}

TEST(ClosedInterval, TimestampsAreAllowed)
{
    using TP = std::chrono::system_clock::time_point;
    ASSERT_NO_THROW({ ClosedInterval<TP>(TP{}, TP{} + std::chrono::seconds{1}); });
}
