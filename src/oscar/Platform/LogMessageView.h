#pragma once

#include <oscar/Platform/LogLevel.h>

#include <chrono>
#include <string_view>

namespace osc
{
    struct LogMessageView final {

        LogMessageView() = default;

        LogMessageView(
            std::string_view logger_name_,
            std::string_view payload_,
            LogLevel level_) :

            logger_name{logger_name_},
            time{std::chrono::system_clock::now()},
            payload{payload_},
            level{level_}
        {}

        std::string_view logger_name;
        std::chrono::system_clock::time_point time;
        std::string_view payload;
        LogLevel level;
    };
}
