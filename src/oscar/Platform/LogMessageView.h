#pragma once

#include <oscar/Platform/LogLevel.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/StringName.h>

#include <chrono>
#include <string_view>

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
