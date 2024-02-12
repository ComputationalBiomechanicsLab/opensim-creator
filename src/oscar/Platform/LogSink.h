#pragma once

#include <oscar/Platform/LogLevel.h>

namespace osc { struct LogMessageView; }

namespace osc
{
    class LogSink {
    protected:
        LogSink() = default;
        LogSink(LogSink const&) = delete;
        LogSink(LogSink&&) noexcept = delete;
        LogSink& operator=(LogSink const&) = delete;
        LogSink& operator=(LogSink&&) noexcept = delete;
    public:
        virtual ~LogSink() noexcept = default;

        void log_message(LogMessageView const& view)
        {
            implLog(view);
        }

        [[nodiscard]] LogLevel getLevel() const
        {
            return m_SinkLevel;
        }

        void setLevel(LogLevel level)
        {
            m_SinkLevel = level;
        }

        [[nodiscard]] bool shouldLog(LogLevel level) const
        {
            return level >= m_SinkLevel;
        }

    private:
        virtual void implLog(LogMessageView const&) = 0;

        LogLevel m_SinkLevel = LogLevel::trace;
    };
}
