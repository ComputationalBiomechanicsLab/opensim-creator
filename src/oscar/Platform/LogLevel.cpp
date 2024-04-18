#include "LogLevel.h"

#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/StringHelpers.h>

#include <array>
#include <cstddef>
#include <iterator>

using namespace osc;

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
    auto const it = find_if(c_LogLevelStrings, [v](std::string_view el)
    {
        return is_equal_case_insensitive(v, el);
    });

    return it != c_LogLevelStrings.end() ?
        FromIndex(std::distance(c_LogLevelStrings.begin(), it)) :
        std::nullopt;
}
