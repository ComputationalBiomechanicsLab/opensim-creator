#include "log.hpp"

#include <cstdio>
#include <exception>
#include <iostream>
#include <mutex>

using namespace osmv;
using namespace osmv::log;

static std::string_view level_names[] OSMV_LOG_LVL_NAMES;
static char const* level_cstring_names[] OSMV_LOG_LVL_NAMES;

std::string_view const& osmv::log::to_string_view(level::Level_enum lvl) noexcept {
    return level_names[lvl];
}

char const* osmv::log::to_c_str(level::Level_enum lvl) noexcept {
    return level_cstring_names[lvl];
}

class Stdout_sink final : public Sink {
    std::mutex mutex;

    void log(Log_msg const& msg) override {
        std::lock_guard g{mutex};
        std::cout << '[' << msg.logger_name << "] [" << to_string_view(msg.level) << "] " << msg.payload << std::endl;
    }
};

std::shared_ptr<Logger> osmv::log::default_logger() noexcept {
    static std::shared_ptr<Logger> ptr = std::make_shared<Logger>("default", std::make_shared<Stdout_sink>());
    return ptr;
}

Logger* osmv::log::default_logger_raw() noexcept {
    return default_logger().get();
}
