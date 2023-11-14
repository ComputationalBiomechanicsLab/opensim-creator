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

    constexpr LogLevel FirstLogLevel() noexcept
    {
        return LogLevel::trace;
    }

    constexpr LogLevel Next(LogLevel lvl) noexcept
    {
        return static_cast<LogLevel>(osc::to_underlying(lvl) + 1);
    }

    constexpr LogLevel LastLogLevel() noexcept
    {
        return LogLevel::critical;
    }

    constexpr size_t ToIndex(LogLevel level) noexcept
    {
        return static_cast<size_t>(level);
    }

    std::optional<LogLevel> FromIndex(size_t) noexcept;
    CStringView ToCStringView(LogLevel);
    std::optional<LogLevel> TryParseAsLogLevel(std::string_view);
}
