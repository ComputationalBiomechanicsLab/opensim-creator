#pragma once

#include "src/utils/circular_buffer.hpp"
#include "src/utils/concurrency.hpp"

#include <chrono>
#include <cstdarg>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace osc::log {
    // this implementation is a gruesome simplification of spdlog: go read spdlog
    // sources if you want to see good software engineering

    namespace level {
        enum Level_enum { trace = 0, debug, info, warn, err, critical, off, NUM_LEVELS };

#define OSC_LOG_LVL_NAMES                                                                                             \
    { "trace", "debug", "info", "warning", "error", "critical", "off" }

        extern std::string_view const name_views[];
        extern char const* const name_cstrings[];
    }

    [[nodiscard]] inline std::string_view const& to_string_view(level::Level_enum lvl) noexcept {
        return level::name_views[lvl];
    }

    [[nodiscard]] inline char const* to_c_str(level::Level_enum lvl) noexcept {
        return level::name_cstrings[lvl];
    }

    // a log message
    //
    // to prevent needless runtime allocs, this does not own its data. See below if you need an
    // owning version
    struct Log_msg final {
        std::string_view logger_name;
        std::chrono::system_clock::time_point time;
        std::string_view payload;
        level::Level_enum level;

        Log_msg() = default;
        Log_msg(std::string_view _logger_name, std::string_view _payload, level::Level_enum _level) :
            logger_name{_logger_name},
            time{std::chrono::system_clock::now()},
            payload{_payload},
            level{_level} {
        }
    };

    // a log message that owns all its data
    //
    // useful if you need to persist a log message somewhere
    struct Owned_log_msg final {
        std::string logger_name;
        std::chrono::system_clock::time_point time;
        std::string payload;
        log::level::Level_enum level;

        Owned_log_msg() = default;

        Owned_log_msg(log::Log_msg const& msg) :
            logger_name{msg.logger_name},
            time{msg.time},
            payload{msg.payload},
            level{msg.level} {
        }
    };

    class Sink {
        level::Level_enum level_{level::info};

    public:
        virtual ~Sink() noexcept = default;
        virtual void log(Log_msg const&) = 0;

        void set_level(level::Level_enum level) noexcept {
            level_ = level;
        }

        [[nodiscard]] level::Level_enum level() const noexcept {
            return level_;
        }

        [[nodiscard]] bool should_log(level::Level_enum level) const noexcept {
            return level >= level_;
        }
    };

    class Logger final {
        std::string name;
        std::vector<std::shared_ptr<Sink>> _sinks;
        level::Level_enum level{level::trace};

    public:
        Logger(std::string _name) : name{std::move(_name)}, _sinks() {
        }

        Logger(std::string _name, std::shared_ptr<Sink> _sink) : name{std::move(_name)}, _sinks{_sink} {
        }

        template<typename... Args>
        void log(level::Level_enum msg_level, char const* fmt, ...) {
            if (msg_level < level) {
                return;
            }

            // create the log message
            char buf[512];
            size_t n = 0;
            {
                va_list args;
                va_start(args, fmt);
                int rv = std::vsnprintf(buf, sizeof(buf), fmt, args);
                va_end(args);

                if (rv < 0) {
                    return;
                }
                n = std::min(static_cast<size_t>(rv), sizeof(buf));
            }
            Log_msg msg{name, std::string_view{buf, n}, msg_level};

            // sink it
            for (auto& sink : _sinks) {
                if (sink->should_log(msg.level)) {
                    sink->log(msg);
                }
            }
        }

        template<typename... Args>
        void trace(char const* fmt, Args const&... args) {
            log(level::trace, fmt, args...);
        }

        template<typename... Args>
        void debug(char const* fmt, Args const&... args) {
            log(level::debug, fmt, args...);
        }

        template<typename... Args>
        void info(char const* fmt, Args const&... args) {
            log(level::info, fmt, args...);
        }

        template<typename... Args>
        void warn(char const* fmt, Args const&... args) {
            log(level::warn, fmt, args...);
        }

        template<typename... Args>
        void error(char const* fmt, Args const&... args) {
            log(level::err, fmt, args...);
        }

        template<typename... Args>
        void critical(char const* fmt, Args const&... args) {
            log(level::critical, fmt, args...);
        }

        [[nodiscard]] std::vector<std::shared_ptr<Sink>> const& sinks() const noexcept {
            return _sinks;
        }

        [[nodiscard]] std::vector<std::shared_ptr<Sink>>& sinks() noexcept {
            return _sinks;
        }
    };

    // global logging API
    std::shared_ptr<Logger> default_logger() noexcept;
    Logger* default_logger_raw() noexcept;

    template<typename... Args>
    inline void log(level::Level_enum level, char const* fmt, Args const&... args) {
        default_logger_raw()->log(level, fmt, args...);
    }

    template<typename... Args>
    inline void trace(char const* fmt, Args const&... args) {
        default_logger_raw()->trace(fmt, args...);
    }

    template<typename... Args>
    inline void debug(char const* fmt, Args const&... args) {
        default_logger_raw()->debug(fmt, args...);
    }

    template<typename... Args>
    void info(char const* fmt, Args const&... args) {
        default_logger_raw()->info(fmt, args...);
    }

    template<typename... Args>
    void warn(char const* fmt, Args const&... args) {
        default_logger_raw()->warn(fmt, args...);
    }

    template<typename... Args>
    void error(char const* fmt, Args const&... args) {
        default_logger_raw()->error(fmt, args...);
    }

    template<typename... Args>
    void critical(char const* fmt, Args const&... args) {
        default_logger_raw()->critical(fmt, args...);
    }

    static constexpr size_t max_traceback_log_messages = 256;

    [[nodiscard]] level::Level_enum get_traceback_level();
    void set_traceback_level(level::Level_enum);
    [[nodiscard]] Mutex_guarded<Circular_buffer<Owned_log_msg, max_traceback_log_messages>>& get_traceback_log();
}
