#include "PerfClock.h"

#include <gtest/gtest.h>

#include <chrono>
#include <ratio>

using namespace osc;

TEST(PerfClock, is_suitable_for_high_performance_measurements)
{
    static_assert(PerfClock::is_steady, "performance clocks should be steady - they shouldn't go backwards just because the caller screwed with their system clock");
    static_assert(std::ratio_less_equal<PerfClock::period, std::nano>(), "performance clocks should be high-resolution enough to capture very very fast events");
}
