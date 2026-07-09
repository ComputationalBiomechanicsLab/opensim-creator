#pragma once

#include <liboscar/platform/logger.h>
#include <liboscar/platform/log_level.h>
#include <liboscar/platform/log_message.h>
#include <liboscar/utilities/circular_buffer.h>
#include <liboscar/utilities/synchronized_value.h>

#include <cstddef>
#include <format>
#include <memory>

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
    void log_message(LogLevel level, std::format_string<Args...> fmt, Args&&... args)
    {
        global_default_logger_raw()->log_message(level, std::move(fmt), std::forward<Args>(args)...);
    }

    template<typename... Args>
    void log_trace(std::format_string<Args...> fmt, Args&&... args)
    {
        global_default_logger_raw()->log_message(LogLevel::trace, std::move(fmt), std::forward<Args>(args)...);
    }

    template<typename... Args>
    void log_debug(std::format_string<Args...> fmt, Args&&... args)
    {
        global_default_logger_raw()->log_message(LogLevel::debug, std::move(fmt), std::forward<Args>(args)...);
    }

    template<typename... Args>
    void log_info(std::format_string<Args...> fmt, Args&&... args)
    {
        global_default_logger_raw()->log_message(LogLevel::info, std::move(fmt), std::forward<Args>(args)...);
    }

    template<typename... Args>
    void log_warn(std::format_string<Args...> fmt, Args&&... args)
    {
        global_default_logger_raw()->log_message(LogLevel::warn, std::move(fmt), std::forward<Args>(args)...);
    }

    template<typename... Args>
    void log_error(std::format_string<Args...> fmt, Args&&... args)
    {
        global_default_logger_raw()->log_message(LogLevel::err, std::move(fmt), std::forward<Args>(args)...);
    }

    template<typename... Args>
    void log_critical(std::format_string<Args...> fmt, Args&&... args)
    {
        global_default_logger_raw()->log_message(LogLevel::critical, std::move(fmt), std::forward<Args>(args)...);
    }

    namespace detail
    {
        static constexpr size_t c_max_log_traceback_messages = 512;
    }

    [[nodiscard]] LogLevel global_get_traceback_level();
    void global_set_traceback_level(LogLevel);
    [[nodiscard]] SynchronizedValue<CircularBuffer<LogMessage, detail::c_max_log_traceback_messages>>& global_get_traceback_log();
}
