#pragma once

#include <liboscar/Platform/LogLevel.h>

namespace osc { class LogMessageView; }

namespace osc
{
    // An abstract base class for an object that can receive (sink) log
    // messages from a `Logger`.
    class LogSink {
    protected:
        LogSink() = default;
        LogSink(const LogSink&) = default;
        LogSink(LogSink&&) noexcept = default;
        LogSink& operator=(const LogSink&) = default;
        LogSink& operator=(LogSink&&) noexcept = default;
    public:
        virtual ~LogSink() noexcept = default;

        LogLevel level() const { return sink_level_; }
        void set_level(LogLevel log_level) { sink_level_ = log_level; }
        bool should_log(LogLevel message_level) const { return message_level >= level(); }
        void sink_message(const LogMessageView& message_view) { impl_sink_message(message_view); }

    private:
        virtual void impl_sink_message(const LogMessageView&) = 0;

        LogLevel sink_level_ = LogLevel::trace;
    };
}
