#pragma once

#include <oscar/Utils/CStringView.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <type_traits>

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
        using Underlying = std::underlying_type_t<LogLevel>;
        return static_cast<LogLevel>(static_cast<Underlying>(lvl) + 1);
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
