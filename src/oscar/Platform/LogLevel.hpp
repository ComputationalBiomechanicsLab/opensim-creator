#pragma once

#include "oscar/Utils/CStringView.hpp"

#include <cstdint>
#include <type_traits>

namespace osc
{
    enum class LogLevel {
        FIRST = 0,
        trace = FIRST,
        debug,
        info,
        warn,
        err,
        critical,
        off,
        NUM_LEVELS,
    };

    CStringView ToCStringView(LogLevel);

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
}
