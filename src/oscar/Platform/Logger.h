#pragma once

#include <oscar/Platform/ILogSink.h>
#include <oscar/Platform/LogMessageView.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/StringName.h>

#include <algorithm>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <memory>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace osc
{
    class Logger final {
    public:
        explicit Logger(std::string_view name) :
            name_{name}
        {}

        Logger(std::string_view name, std::shared_ptr<ILogSink> sink) :
            name_{name},
            log_sinks_{std::move(sink)}
        {}

        void log_message(LogLevel message_log_level, CStringView fmt, ...)
        {
            if (message_log_level < log_level_) {
                return;  // the message's level is too low for this logger
            }

            auto it = std::ranges::find_if(log_sinks_, [message_log_level](const auto& sink) { return sink->should_log(message_log_level); });
            if (it == log_sinks_.end()) {
                return;  // no sink in this logger will consume the message
            }

            // else: there exists at least one sink that wants the message

            // format the format string with the arguments
            std::vector<char> formatted_buffer(2048);
            size_t n = 0;
            {
                va_list args;
                va_start(args, fmt);
                int rv = std::vsnprintf(formatted_buffer.data(), formatted_buffer.size(), fmt.c_str(), args);
                va_end(args);

                if (rv <= 0) {
                    return;  // formatting error: exit early
                }

                n = min(static_cast<size_t>(rv), formatted_buffer.size()-1);
            }

            // create a readonly view of the message that sinks _may_ consume
            LogMessageView view{name_, CStringView{formatted_buffer.data(), n}, message_log_level};

            // sink the message
            for (; it != log_sinks_.end(); ++it) {
                if ((*it)->should_log(view.level())) {
                    (*it)->sink_message(view);
                }
            }
        }

        template<typename... Args>
        void log_trace(CStringView fmt, const Args&... args)
        {
            log_message(LogLevel::trace, fmt, args...);
        }

        template<typename... Args>
        void log_debug(CStringView fmt, const Args&... args)
        {
            log_message(LogLevel::debug, fmt, args...);
        }

        template<typename... Args>
        void log_info(CStringView fmt, const Args&... args)
        {
            log_message(LogLevel::info, fmt, args...);
        }

        template<typename... Args>
        void log_warn(CStringView fmt, const Args&... args)
        {
            log_message(LogLevel::warn, fmt, args...);
        }

        template<typename... Args>
        void log_error(CStringView fmt, const Args&... args)
        {
            log_message(LogLevel::err, fmt, args...);
        }

        template<typename... Args>
        void log_critical(CStringView fmt, const Args&... args)
        {
            log_message(LogLevel::critical, fmt, args...);
        }

        const std::vector<std::shared_ptr<ILogSink>>& sinks() const { return log_sinks_; }
        std::vector<std::shared_ptr<ILogSink>>& sinks() { return log_sinks_; }

        LogLevel level() const { return log_level_; }
        void set_level(LogLevel level_) { log_level_ = level_; }

    private:
        StringName name_;
        std::vector<std::shared_ptr<ILogSink>> log_sinks_;
        LogLevel log_level_ = LogLevel::DEFAULT;
    };
}
