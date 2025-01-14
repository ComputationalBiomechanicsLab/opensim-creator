#pragma once

#include <liboscar/Utils/CStringView.h>

#include <iosfwd>
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
    std::ostream& operator<<(std::ostream&, LogLevel);
}
