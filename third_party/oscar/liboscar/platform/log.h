#pragma once

#include <liboscar/platform/logger.h>
#include <liboscar/platform/log_level.h>
#include <liboscar/platform/log_message.h>
#include <liboscar/utils/circular_buffer.h>
#include <liboscar/utils/c_string_view.h>
#include <liboscar/utils/synchronized_value.h>

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
        global_default_logger_raw()->log_trace(fmt, args...);
    }

    template<typename... Args>
    inline void log_debug(CStringView fmt, const Args&... args)
    {
        global_default_logger_raw()->log_debug(fmt, args...);
    }

    template<typename... Args>
    void log_info(CStringView fmt, const Args&... args)
    {
        global_default_logger_raw()->log_info(fmt, args...);
    }

    template<typename... Args>
    void log_warn(CStringView fmt, const Args&... args)
    {
        global_default_logger_raw()->log_warn(fmt, args...);
    }

    template<typename... Args>
    void log_error(CStringView fmt, const Args&... args)
    {
        global_default_logger_raw()->log_error(fmt, args...);
    }

    template<typename... Args>
    void log_critical(CStringView fmt, const Args&... args)
    {
        global_default_logger_raw()->log_critical(fmt, args...);
    }

    namespace detail
    {
        static constexpr size_t c_max_log_traceback_messages = 512;
    }

    [[nodiscard]] LogLevel global_get_traceback_level();
    void global_set_traceback_level(LogLevel);
    [[nodiscard]] SynchronizedValue<CircularBuffer<LogMessage, detail::c_max_log_traceback_messages>>& global_get_traceback_log();
}
