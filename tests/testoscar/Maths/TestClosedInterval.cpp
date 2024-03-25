#include <oscar/Maths/ClosedInterval.h>

#include <gtest/gtest.h>

#include <chrono>

using namespace osc;

TEST(ClosedInterval, CanConstructForInts)
{
    ASSERT_NO_THROW({ ClosedInterval(0, 1); });
}

TEST(ClosedInterval, ReversingOrderIsAllowed)
{
    ASSERT_NO_THROW({ ClosedInterval(1, 0); });
}

TEST(ClosedInterval, TimestampsAreAllowed)
{
    ASSERT_NO_THROW({ ClosedInterval(std::chrono::system_clock::time_point{}, std::chrono::system_clock::time_point{} + std::chrono::seconds{1}); });
}
