#include <oscar/Utils/ChronoHelpers.h>

#include <gtest/gtest.h>

#include <chrono>

using namespace osc;
using namespace std::literals;

TEST(ChronoHelpers, lerpDurationsWorksAsExpected)
{
    static_assert(lerp(1s, 10s, 0.0f) == 1s);
    static_assert(lerp(1s, 10s, 1.0f) == 10s);
    static_assert(lerp(1s, 10s, 0.5f) == 5s);  // ultimately, converts back to integer representation
}

TEST(ChronoHelpers, lerpTimePointsWorksAsExpected)
{
    using TimePoint = std::chrono::system_clock::time_point;
    static_assert(lerp(TimePoint{}, TimePoint{1s}, 0.0f) == TimePoint{});
    static_assert(lerp(TimePoint{}, TimePoint{10s}, 1.0f) == TimePoint{10s});
    static_assert(lerp(TimePoint{}, TimePoint{10s}, 0.5f) == TimePoint{5s});
}
