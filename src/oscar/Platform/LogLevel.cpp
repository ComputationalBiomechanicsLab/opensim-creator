#include "LogLevel.h"

#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/EnumHelpers.h>
#include <oscar/Utils/StringHelpers.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <iostream>
#include <iterator>
#include <ranges>

using namespace osc;
namespace rgs = std::ranges;

namespace
{
    constexpr auto c_log_level_strings = std::to_array<CStringView>(
    {
        "trace",
        "debug",
        "info",
        "warning",
        "error",
        "critical",
        "off",
    });
    static_assert(c_log_level_strings.size() == static_cast<size_t>(LogLevel::NUM_OPTIONS));
}

CStringView osc::to_cstringview(LogLevel level)
{
    return c_log_level_strings.at(to_index(level));
}

std::optional<LogLevel> osc::try_parse_as_log_level(std::string_view str)
{
    const auto it = rgs::find_if(c_log_level_strings, std::bind_front(is_equal_case_insensitive, str));
    if (it == c_log_level_strings.end()) {
        return std::nullopt;
    }

    return from_index<LogLevel>(std::distance(c_log_level_strings.begin(), it));
}

std::ostream& osc::operator<<(std::ostream& out, LogLevel level)
{
    return out << to_cstringview(level);
}
