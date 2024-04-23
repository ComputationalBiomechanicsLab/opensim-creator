#pragma once

#include <oscar/Platform/LogLevel.h>

namespace osc { class LogMessageView; }

namespace osc
{
    class LogSink {
    protected:
        LogSink() = default;
        LogSink(const LogSink&) = delete;
        LogSink(LogSink&&) noexcept = delete;
        LogSink& operator=(const LogSink&) = delete;
        LogSink& operator=(LogSink&&) noexcept = delete;
    public:
        virtual ~LogSink() noexcept = default;

        void log_message(const LogMessageView& view) { impl_log_message(view); }

        LogLevel level() const { return sink_level_; }
        void set_level(LogLevel level) { sink_level_ = level; }

        bool should_log(LogLevel level) const { return level >= sink_level_; }

    private:
        virtual void impl_log_message(const LogMessageView&) = 0;

        LogLevel sink_level_ = LogLevel::trace;
    };
}
