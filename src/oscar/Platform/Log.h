#pragma once

#include <oscar/Platform/Logger.h>
#include <oscar/Platform/LogLevel.h>
#include <oscar/Platform/LogMessage.h>
#include <oscar/Utils/CircularBuffer.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/SynchronizedValue.h>

#include <cstddef>
#include <memory>

// log: logging implementation
//
// this implementation takes heavy inspiration from `spdlog`

namespace osc
{
    // global logging API
    std::shared_ptr<Logger> defaultLogger();
    Logger* defaultLoggerRaw();

    inline LogLevel log_level()
    {
        return defaultLoggerRaw()->get_level();
    }

    template<typename... Args>
    inline void log_message(LogLevel level, CStringView fmt, Args const&... args)
    {
        defaultLoggerRaw()->log_message(level, fmt, args...);
    }

    template<typename... Args>
    inline void log_trace(CStringView fmt, Args const&... args)
    {
        defaultLoggerRaw()->log_trace(fmt, args...);
    }

    template<typename... Args>
    inline void log_debug(CStringView fmt, Args const&... args)
    {
        defaultLoggerRaw()->log_debug(fmt, args...);
    }

    template<typename... Args>
    void log_info(CStringView fmt, Args const&... args)
    {
        defaultLoggerRaw()->log_info(fmt, args...);
    }

    template<typename... Args>
    void log_warn(CStringView fmt, Args const&... args)
    {
        defaultLoggerRaw()->log_warn(fmt, args...);
    }

    template<typename... Args>
    void log_error(CStringView fmt, Args const&... args)
    {
        defaultLoggerRaw()->log_error(fmt, args...);
    }

    template<typename... Args>
    void log_critical(CStringView fmt, Args const&... args)
    {
        defaultLoggerRaw()->log_critical(fmt, args...);
    }

    namespace detail
    {
        constexpr static size_t c_MaxLogTracebackMessages = 1024;
    }

    [[nodiscard]] LogLevel getTracebackLevel();
    void setTracebackLevel(LogLevel);
    [[nodiscard]] SynchronizedValue<CircularBuffer<LogMessage, detail::c_MaxLogTracebackMessages>>& getTracebackLog();
}
