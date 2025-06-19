#include "ChronoHelpers.h"

#include <chrono>
#include <sstream>
#include <string>
#include <utility>

using namespace std::literals;

std::string osc::to_iso8601_timestamp(std::chrono::zoned_seconds t)
{
    std::stringstream ss;
    const std::chrono::local_seconds local_seconds = t.get_local_time();
    const auto local_days = std::chrono::floor<std::chrono::days>(local_seconds);

    // emit calendar part
    const std::chrono::year_month_day ymd{local_days};
    ss << std::setw(4) << std::setfill('0') << static_cast<int>(ymd.year());
    ss << '-';
    ss << std::setw(2) << std::setfill('0') << static_cast<unsigned>(ymd.month());
    ss << '-';
    ss << std::setw(2) << std::setfill('0') << static_cast<unsigned>(ymd.day());

    ss << ' ';  // calendar-to-time delimiter

    // emit time part
    const std::chrono::hh_mm_ss tod{local_seconds - local_days};
    ss << std::setw(2) << std::setfill('0') << tod.hours().count();
    ss << ':';
    ss << std::setw(2) << std::setfill('0') << tod.minutes().count();
    ss << ':';
    ss << std::setw(2) << std::setfill('0') << tod.seconds().count();

    // emit time zone part
    const std::chrono::seconds offset = t.get_info().offset;
    if (offset == 0s) {
        ss << 'Z';  // time is in UTC, clarify it with the Z suffix
    }
    else {
        // emit offset that should be applied to civil time
        ss << (offset > 0s ? '+' : '-');
        const std::chrono::hh_mm_ss offset_hhmmss{offset};
        ss << std::setw(2) << std::setfill('0') << offset_hhmmss.hours().count();
        if (offset_hhmmss.minutes().count() > 0) {
            ss << ':';
            ss << std::setw(2) << std::setfill('0') << offset_hhmmss.minutes().count();
        }
    }

    // return string
    return std::move(ss).str();
}
