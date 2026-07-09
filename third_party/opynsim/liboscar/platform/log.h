#pragma once

#include <liboscar/platform/logger.h>
#include <liboscar/platform/log_level.h>
#include <liboscar/platform/log_message.h>
#include <liboscar/utilities/circular_buffer.h>
#include <liboscar/utilities/synchronized_value.h>

#include <cstddef>
#include <format>
#include <memory>
#include <string_view>

namespace osc
{
    // global logging API
    std::shared_ptr<Logger> global_default_logger();
    Logger* global_default_logger_raw();

    inline LogLevel log_level()
    {
        return global_default_logger_raw()->level();
    }

    inline void log_message(LogLevel level, std::string_view message)
    {
        global_default_logger_raw()->log_message(level, message);
    }

    template<typename... Args>
    void log_message(LogLevel level, std::format_string<Args...> fmt, Args&&... args)
    {
        global_default_logger_raw()->log_message(level, fmt, std::forward<Args>(args)...);
    }

    inline void log_trace(std::string_view message)
    {
        log_message(LogLevel::trace, message);
    }

    template<typename... Args>
    void log_trace(std::format_string<Args...> fmt, Args&&... args)
    {
        log_message(LogLevel::trace, fmt, std::forward<Args>(args)...);
    }

    inline void log_debug(std::string_view message)
    {
        log_message(LogLevel::debug, message);
    }

    template<typename... Args>
    void log_debug(std::format_string<Args...> fmt, Args&&... args)
    {
        log_message(LogLevel::debug, fmt, std::forward<Args>(args)...);
    }

    inline void log_info(std::string_view message)
    {
        log_message(LogLevel::info, message);
    }

    template<typename... Args>
    void log_info(std::format_string<Args...> fmt, Args&&... args)
    {
        log_message(LogLevel::info, fmt, std::forward<Args>(args)...);
    }

    inline void log_warn(std::string_view message)
    {
        log_message(LogLevel::warn, message);
    }

    template<typename... Args>
    void log_warn(std::format_string<Args...> fmt, Args&&... args)
    {
        log_message(LogLevel::warn, fmt, std::forward<Args>(args)...);
    }

    inline void log_error(std::string_view message)
    {
        log_message(LogLevel::err, message);
    }

    template<typename... Args>
    void log_error(std::format_string<Args...> fmt, Args&&... args)
    {
        log_message(LogLevel::err, fmt, std::forward<Args>(args)...);
    }

    inline void log_critical(std::string_view message)
    {
        log_message(LogLevel::critical, message);
    }

    template<typename... Args>
    void log_critical(std::format_string<Args...> fmt, Args&&... args)
    {
        log_message(LogLevel::critical, fmt, std::forward<Args>(args)...);
    }

    namespace detail
    {
        static constexpr size_t c_max_log_traceback_messages = 512;
    }

    [[nodiscard]] LogLevel global_get_traceback_level();
    void global_set_traceback_level(LogLevel);
    [[nodiscard]] SynchronizedValue<CircularBuffer<LogMessage, detail::c_max_log_traceback_messages>>& global_get_traceback_log();
}
