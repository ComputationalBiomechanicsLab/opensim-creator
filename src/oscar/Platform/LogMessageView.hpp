#pragma once

#include "oscar/Platform/LogLevel.hpp"

#include <chrono>
#include <string_view>

namespace osc
{
    struct LogMessageView final {

        LogMessageView() = default;

        LogMessageView(
            std::string_view loggerName_,
            std::string_view payload_,
            LogLevel level_) :
            loggerName{loggerName_},
            time{std::chrono::system_clock::now()},
            payload{payload_},
            level{level_}
        {
        }

        std::string_view loggerName;
        std::chrono::system_clock::time_point time;
        std::string_view payload;
        LogLevel level;
    };
}
