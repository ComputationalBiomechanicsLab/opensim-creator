#pragma once

#include <oscar/Shims/Cpp23/utility.h>
#include <oscar/Utils/CStringView.h>

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
        NUM_OPTIONS,
        DEFAULT = info,
    };

    CStringView to_cstringview(LogLevel);
    std::optional<LogLevel> try_parse_as_log_level(std::string_view);
}
