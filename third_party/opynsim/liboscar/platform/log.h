#pragma once

#include <liboscar/platform/logger.h>
#include <liboscar/platform/log_level.h>
#include <liboscar/platform/log_message.h>
#include <liboscar/utilities/circular_buffer.h>
#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/synchronized_value.h>

#include <cstddef>
#include <memory>

// log: logging implementation
//
// this implementation takes heavy inspiration from `spdlog`

namespace osc
{
    // global logging API
    std::shared_ptr<Logger> global_default_logger();
    Logger* global_default_logger_raw();

    inline LogLevel log_level()
    {
        return global_default_logger_raw()->level();
    }

    template<typename... Args>
    inline void log_message(LogLevel level, CStringView fmt, const Args&... args)
    {
        global_default_logger_raw()->log_message(level, fmt, args...);
    }

    template<typename... Args>
    inline void log_trace(CStringView fmt, const Args&... args)
    {
        global_default_logger_raw()->log_message(LogLevel::trace, fmt, args...);
    }

    template<typename... Args>
    inline void log_debug(CStringView fmt, const Args&... args)
    {
        global_default_logger_raw()->log_message(LogLevel::debug, fmt, args...);
    }

    template<typename... Args>
    void log_info(CStringView fmt, const Args&... args)
    {
        global_default_logger_raw()->log_message(LogLevel::info, fmt, args...);
    }

    template<typename... Args>
    void log_warn(CStringView fmt, const Args&... args)
    {
        global_default_logger_raw()->log_message(LogLevel::warn, fmt, args...);
    }

    template<typename... Args>
    void log_error(CStringView fmt, const Args&... args)
    {
        global_default_logger_raw()->log_message(LogLevel::err, fmt, args...);
    }

    template<typename... Args>
    void log_critical(CStringView fmt, const Args&... args)
    {
        global_default_logger_raw()->log_message(LogLevel::critical, fmt, args...);
    }

    namespace detail
    {
        static constexpr size_t c_max_log_traceback_messages = 512;
    }

    [[nodiscard]] LogLevel global_get_traceback_level();
    void global_set_traceback_level(LogLevel);
    [[nodiscard]] SynchronizedValue<CircularBuffer<LogMessage, detail::c_max_log_traceback_messages>>& global_get_traceback_log();
}
