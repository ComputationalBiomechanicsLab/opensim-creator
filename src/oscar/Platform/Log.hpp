#pragma once

#include <oscar/Platform/Logger.hpp>
#include <oscar/Platform/LogLevel.hpp>
#include <oscar/Platform/LogMessage.hpp>
#include <oscar/Platform/LogMessageView.hpp>
#include <oscar/Platform/LogSink.hpp>
#include <oscar/Utils/CircularBuffer.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/SynchronizedValue.hpp>

#include <nonstd/span.hpp>

#include <algorithm>
#include <chrono>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// log: logging implementation
//
// this implementation takes heavy inspiration from `spdlog`

namespace osc::log
{
    // global logging API
    std::shared_ptr<Logger> defaultLogger() noexcept;
    Logger* defaultLoggerRaw() noexcept;

    template<typename... Args>
    inline void log(LogLevel level, CStringView fmt, Args const&... args)
    {
        defaultLoggerRaw()->log(level, fmt, args...);
    }

    template<typename... Args>
    inline void trace(CStringView fmt, Args const&... args)
    {
        defaultLoggerRaw()->trace(fmt, args...);
    }

    template<typename... Args>
    inline void debug(CStringView fmt, Args const&... args)
    {
        defaultLoggerRaw()->debug(fmt, args...);
    }

    template<typename... Args>
    void info(CStringView fmt, Args const&... args)
    {
        defaultLoggerRaw()->info(fmt, args...);
    }

    template<typename... Args>
    void warn(CStringView fmt, Args const&... args)
    {
        defaultLoggerRaw()->warn(fmt, args...);
    }

    template<typename... Args>
    void error(CStringView fmt, Args const&... args)
    {
        defaultLoggerRaw()->error(fmt, args...);
    }

    template<typename... Args>
    void critical(CStringView fmt, Args const&... args)
    {
        defaultLoggerRaw()->critical(fmt, args...);
    }

    constexpr static size_t c_MaxLogTracebackMessages = 1024;

    [[nodiscard]] LogLevel getTracebackLevel();
    void setTracebackLevel(LogLevel);
    [[nodiscard]] SynchronizedValue<CircularBuffer<LogMessage, c_MaxLogTracebackMessages>>& getTracebackLog();
}
