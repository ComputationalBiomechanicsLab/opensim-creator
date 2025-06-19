#include "ChronoHelpers.h"

#include <gtest/gtest.h>

#include <chrono>

using namespace osc;
using namespace std::literals;

TEST(lerp, works_on_std_chrono_durations)
{
    static_assert(lerp(1s, 10s, 0.0f) == 1s);
    static_assert(lerp(1s, 10s, 1.0f) == 10s);
    static_assert(lerp(1s, 10s, 0.5f) == 5s);  // ultimately, converts back to integer representation
}

TEST(lerp, works_on_std_chrono_time_points)
{
    using TimePoint = std::chrono::system_clock::time_point;
    static_assert(lerp(TimePoint{}, TimePoint{1s}, 0.0f) == TimePoint{});
    static_assert(lerp(TimePoint{}, TimePoint{10s}, 1.0f) == TimePoint{10s});
    static_assert(lerp(TimePoint{}, TimePoint{10s}, 0.5f) == TimePoint{5s});
}

TEST(to_iso8601_timestamp, contains_expected_year_string)
{
    const std::chrono::zoned_seconds t{std::chrono::current_zone(), std::chrono::local_days{2025y/06/19}};
    ASSERT_TRUE(to_iso8601_timestamp(t).contains("2025-06-19")) << to_iso8601_timestamp(t) << ": does not contain date?";
}

TEST(to_iso8601_timestamp, contains_expected_time)
{
    const auto t = std::chrono::local_seconds{std::chrono::local_days{2025y/06/19}} + (10h + 15min + 5s);
    const auto zt = std::chrono::zoned_seconds{std::chrono::current_zone(), t};
    ASSERT_TRUE(to_iso8601_timestamp(zt).contains("10:15:05")) << to_iso8601_timestamp(zt) << ": does not coontain time?";
}

TEST(to_iso8601_timestamp, contains_timestamp)
{
    const auto t = std::chrono::local_seconds{std::chrono::local_days{2025y/06/19}} + (10h + 15min + 5s);
    const auto zt = std::chrono::zoned_seconds{"Europe/Amsterdam", t};
    ASSERT_TRUE(to_iso8601_timestamp(zt).contains("+02")) << to_iso8601_timestamp(zt) << ": does not contain timestamp?";
}

TEST(to_iso8601_timestamp, has_expected_content)
{
    const auto t = std::chrono::local_seconds{std::chrono::local_days{2025y/06/19}} + (10h + 15min + 5s);
    const auto zt = std::chrono::zoned_seconds{"Europe/Amsterdam", t};
    ASSERT_EQ(to_iso8601_timestamp(zt), "2025-06-19 10:15:05+02");
}
