#pragma once

#include <liboscar/platform/log_level.h>
#include <liboscar/utils/CStringView.h>
#include <liboscar/utils/StringName.h>

#include <chrono>

namespace osc
{
    // a non-owning view into the contents of a log message
    class LogMessageView final {
    public:
        LogMessageView() = default;

        LogMessageView(
            StringName const& logger_name,
            CStringView payload,
            LogLevel level) :

            logger_name_{logger_name},
            time_{std::chrono::system_clock::now()},
            payload_{payload},
            level_{level}
        {}

        StringName const& logger_name() const { return logger_name_; }
        std::chrono::system_clock::time_point time() const { return time_; }
        CStringView payload() const { return payload_; }
        LogLevel level() const { return level_; }

    private:
        StringName logger_name_;
        std::chrono::system_clock::time_point time_;
        CStringView payload_;
        LogLevel level_;
    };
}
