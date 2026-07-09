#pragma once

#include <liboscar/platform/log_message_view.h>
#include <liboscar/platform/log_sink.h>
#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/string_name.h>

#include <algorithm>
#include <format>
#include <memory>
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

        Logger(std::string_view name, std::shared_ptr<LogSink> sink) :
            name_{name},
            log_sinks_{std::move(sink)}
        {}

        template<class... Args>
        void log_message(LogLevel log_level, std::format_string<Args...> fmt, Args&&... args)
        {
            if (log_level < log_level_) {
                return;  // the message's level is too low for this logger
            }

            auto it = std::ranges::find_if(log_sinks_, [log_level](const auto& sink) { return sink->should_log(log_level); });
            if (it == log_sinks_.end()) {
                return;  // no sink in this logger will consume the message
            }

            // else: there exists at least one sink that wants the message

            // format the format string with the arguments
           const auto message = std::format(std::move(fmt), std::forward<Args>(args)...);

            // create a readonly view of the message that sinks _may_ consume
            LogMessageView view{name_, CStringView{message}, log_level};

            // sink the message
            for (; it != log_sinks_.end(); ++it) {
                if ((*it)->should_log(view.level())) {
                    (*it)->sink_message(view);
                }
            }
        }

        const std::vector<std::shared_ptr<LogSink>>& sinks() const { return log_sinks_; }
        std::vector<std::shared_ptr<LogSink>>& sinks() { return log_sinks_; }

        LogLevel level() const { return log_level_; }
        void set_level(LogLevel level_) { log_level_ = level_; }

    private:
        StringName name_;
        std::vector<std::shared_ptr<LogSink>> log_sinks_;
        LogLevel log_level_ = LogLevel::DEFAULT;
    };
}
