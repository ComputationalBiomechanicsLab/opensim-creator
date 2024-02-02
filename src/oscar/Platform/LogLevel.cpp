#include "LogLevel.hpp"

#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/StringHelpers.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <iterator>

using osc::CStringView;
using osc::LogLevel;

namespace
{
    constexpr auto c_LogLevelStrings = std::to_array<CStringView>(
    {
        "trace",
        "debug",
        "info",
        "warning",
        "error",
        "critical",
        "off",
    });
    static_assert(c_LogLevelStrings.size() == static_cast<size_t>(LogLevel::NUM_LEVELS));
}

CStringView osc::ToCStringView(LogLevel level)
{
    return c_LogLevelStrings.at(static_cast<ptrdiff_t>(level));
}

std::optional<LogLevel> osc::FromIndex(size_t i)
{
    if (ToIndex(FirstLogLevel()) <= i && i <= ToIndex(LastLogLevel()))
    {
        return static_cast<LogLevel>(i);
    }
    else
    {
        return std::nullopt;
    }
}

std::optional<LogLevel> osc::TryParseAsLogLevel(std::string_view v)
{
    auto const pred = [v](std::string_view el)
    {
        return IsEqualCaseInsensitive(v, el);
    };

    auto const it = std::find_if(c_LogLevelStrings.begin(), c_LogLevelStrings.end(), pred);

    return it != c_LogLevelStrings.end() ?
        FromIndex(std::distance(c_LogLevelStrings.begin(), it)) :
        std::nullopt;
}
