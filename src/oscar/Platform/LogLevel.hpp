#pragma once

#include <oscar/Shims/Cpp23/utility.hpp>
#include <oscar/Utils/CStringView.hpp>

#include <cstddef>
#include <optional>
#include <string_view>

namespace osc
{
    enum class LogLevel {
        trace = 0,
        debug,
        info,
        warn,
        err,
        critical,
        off,
        NUM_LEVELS,
        DEFAULT = info,
    };

    constexpr LogLevel FirstLogLevel()
    {
        return LogLevel::trace;
    }

    constexpr LogLevel Next(LogLevel lvl)
    {
        return static_cast<LogLevel>(osc::to_underlying(lvl) + 1);
    }

    constexpr LogLevel LastLogLevel()
    {
        return LogLevel::critical;
    }

    constexpr size_t ToIndex(LogLevel level)
    {
        return static_cast<size_t>(level);
    }

    std::optional<LogLevel> FromIndex(size_t);
    CStringView ToCStringView(LogLevel);
    std::optional<LogLevel> TryParseAsLogLevel(std::string_view);
}
