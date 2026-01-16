#pragma once

#include <liboscar/platform/log_level.h>
#include <liboscar/platform/log_message_view.h>
#include <liboscar/utils/c_string_view.h>
#include <liboscar/utils/string_name.h>

#include <chrono>
#include <string>

namespace osc
{
    // a log message
    class LogMessage final {
    public:
        LogMessage() = default;

        explicit LogMessage(const LogMessageView& view) :
            logger_name_{view.logger_name()},
            time_{view.time()},
            payload_{view.payload()},
            level_{view.level()}
        {}

        StringName const& logger_name() const { return logger_name_; }
        std::chrono::system_clock::time_point time() const { return time_; }
        CStringView payload() const { return payload_; }
        LogLevel level() const { return level_; }

    private:
        StringName logger_name_;
        std::chrono::system_clock::time_point time_;
        std::string payload_;
        LogLevel level_;
    };
}
