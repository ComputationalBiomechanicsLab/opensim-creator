#pragma once

#include <oscar/Platform/LogSink.hpp>
#include <oscar/Platform/LogMessageView.hpp>
#include <oscar/Utils/CStringView.hpp>

#include <algorithm>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace osc
{
    class Logger final {
    public:
        Logger(std::string _name) :
            m_Name{std::move(_name)},
            m_Sinks()
        {}

        Logger(std::string _name, std::shared_ptr<LogSink> _sink) :
            m_Name{std::move(_name)},
            m_Sinks{_sink}
        {}

        void log_message(LogLevel msgLvl, CStringView fmt, ...)
        {
            if (msgLvl < level)
            {
                return;
            }

            // create a not-allocated view of the log message that
            // the sinks _may_ consume
            std::vector<char> buf(2048);
            size_t n = 0;
            {
                va_list args;
                va_start(args, fmt);
                int rv = std::vsnprintf(buf.data(), buf.size(), fmt.c_str(), args);
                va_end(args);

                if (rv <= 0)
                {
                    return;
                }

                n = std::min(static_cast<size_t>(rv), buf.size()-1);
            }
            LogMessageView view{m_Name, std::string_view{buf.data(), n}, msgLvl};

            // sink it
            for (auto& sink : m_Sinks)
            {
                if (sink->shouldLog(view.level))
                {
                    sink->log_message(view);
                }
            }
        }

        template<typename... Args>
        void log_trace(CStringView fmt, Args const&... args)
        {
            log_message(LogLevel::trace, fmt, args...);
        }

        template<typename... Args>
        void log_debug(CStringView fmt, Args const&... args)
        {
            log_message(LogLevel::debug, fmt, args...);
        }

        template<typename... Args>
        void log_info(CStringView fmt, Args const&... args)
        {
            log_message(LogLevel::info, fmt, args...);
        }

        template<typename... Args>
        void log_warn(CStringView fmt, Args const&... args)
        {
            log_message(LogLevel::warn, fmt, args...);
        }

        template<typename... Args>
        void log_error(CStringView fmt, Args const&... args)
        {
            log_message(LogLevel::err, fmt, args...);
        }

        template<typename... Args>
        void log_critical(CStringView fmt, Args const&... args)
        {
            log_message(LogLevel::critical, fmt, args...);
        }

        [[nodiscard]] std::vector<std::shared_ptr<LogSink>> const& sinks() const
        {
            return m_Sinks;
        }

        [[nodiscard]] std::vector<std::shared_ptr<LogSink>>& sinks()
        {
            return m_Sinks;
        }

        [[nodiscard]] LogLevel get_level() const
        {
            return level;
        }

        void set_level(LogLevel level_)
        {
            level = level_;
        }

    private:
        std::string m_Name;
        std::vector<std::shared_ptr<LogSink>> m_Sinks;
        LogLevel level = LogLevel::DEFAULT;
    };
}
