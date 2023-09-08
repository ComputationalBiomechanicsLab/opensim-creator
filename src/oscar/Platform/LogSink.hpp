#pragma once

#include "oscar/Platform/LogLevel.hpp"

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

        void log(LogMessageView const& view)
        {
            implLog(view);
        }

        void set_level(LogLevel level) noexcept
        {
            m_SinkLevel = level;
        }

        [[nodiscard]] LogLevel level() const noexcept
        {
            return m_SinkLevel;
        }

        [[nodiscard]] bool should_log(LogLevel level) const noexcept
        {
            return level >= m_SinkLevel;
        }

    private:
        virtual void implLog(LogMessageView const&) = 0;

        LogLevel m_SinkLevel = LogLevel::trace;
    };
}
