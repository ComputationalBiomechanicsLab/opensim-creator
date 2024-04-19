#pragma once

#include <oscar/Platform/LogLevel.h>
#include <oscar/Platform/LogMessageView.h>

#include <chrono>
#include <string>

namespace osc
{
    // a log message that owns all its data
    //
    // useful if you need to persist a log message somewhere
    struct LogMessage final {

        LogMessage() = default;

        LogMessage(const LogMessageView& view) :
            logger_name{view.logger_name},
            time{view.time},
            payload{view.payload},
            level{view.level}
        {}

        std::string logger_name;
        std::chrono::system_clock::time_point time;
        std::string payload;
        LogLevel level;
    };
}
