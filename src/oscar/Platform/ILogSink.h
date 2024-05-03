#pragma once

#include <oscar/Platform/LogLevel.h>

namespace osc { class LogMessageView; }

namespace osc
{
    class ILogSink {
    protected:
        ILogSink() = default;
        ILogSink(const ILogSink&) = default;
        ILogSink(ILogSink&&) noexcept = default;
        ILogSink& operator=(const ILogSink&) = default;
        ILogSink& operator=(ILogSink&&) noexcept = default;
    public:
        virtual ~ILogSink() noexcept = default;

        void sink_message(const LogMessageView& view) { impl_sink_message(view); }
        LogLevel level() const { return impl_level(); }
        void set_level(LogLevel level) { impl_set_level(level); }

        bool should_log(LogLevel message_level) { return message_level >= level(); }
    private:
        virtual void impl_sink_message(const LogMessageView&) = 0;
        virtual LogLevel impl_level() const = 0;
        virtual void impl_set_level(LogLevel) = 0;
    };
}
