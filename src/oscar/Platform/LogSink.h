#pragma once

#include <oscar/Platform/ILogSink.h>
#include <oscar/Platform/LogLevel.h>

namespace osc
{
    class LogSink : public ILogSink {
    private:
        LogLevel impl_level() const final { return sink_level_; }
        void impl_set_level(LogLevel level) final { sink_level_ = level; }

        LogLevel sink_level_ = LogLevel::trace;
    };
}
