#pragma once

#include "src/log.hpp"
#include "src/utils/circular_buffer.hpp"
#include "src/utils/concurrency.hpp"

#include <chrono>
#include <string>

namespace osmv {
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

    static constexpr size_t max_traceback_log_messages = 256;

    void init_traceback_log();
    [[nodiscard]] Mutex_guarded<Circular_buffer<Owned_log_msg, max_traceback_log_messages>>& get_traceback_log();
}
