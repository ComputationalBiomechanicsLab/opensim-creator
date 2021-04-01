#include "log.hpp"

#include <cstdio>
#include <exception>
#include <iostream>
#include <mutex>

using namespace osmv;
using namespace osmv::log;

class Stdout_sink final : public Sink {
    std::mutex mutex;

    void log(Log_msg const& msg) override {
        std::lock_guard g{mutex};
        std::cout << '[' << msg.logger_name << "] [" << to_string_view(msg.level) << "] " << msg.payload << std::endl;
    }
};

struct Circular_log_sink final : public log::Sink {
    Mutex_guarded<Circular_buffer<Owned_log_msg, max_traceback_log_messages>> storage;

    void log(log::Log_msg const& msg) override {
        auto l = storage.lock();
        l->emplace_back(msg);
    }
};

static std::shared_ptr<Logger> create_default_sink() {
    return std::make_shared<Logger>("default", std::make_shared<Stdout_sink>());
}

static std::shared_ptr<Circular_log_sink> create_traceback_sink() {
    auto rv = std::make_shared<Circular_log_sink>();
    log::default_logger_raw()->sinks().push_back(rv);
    return rv;
}

std::string_view const osmv::log::level::name_views[] OSMV_LOG_LVL_NAMES;
char const* const osmv::log::level::name_cstrings[] OSMV_LOG_LVL_NAMES;
static std::shared_ptr<Logger> default_sink = create_default_sink();
static std::shared_ptr<Circular_log_sink> traceback_sink = create_traceback_sink();

std::shared_ptr<Logger> osmv::log::default_logger() noexcept {
    return default_sink;
}

Logger* osmv::log::default_logger_raw() noexcept {
    return default_logger().get();
}

level::Level_enum osmv::log::get_traceback_level() {
    return traceback_sink->level();
}

void osmv::log::set_traceback_level(level::Level_enum lvl) {
    traceback_sink->set_level(lvl);
}

[[nodiscard]] Mutex_guarded<Circular_buffer<Owned_log_msg, max_traceback_log_messages>>&
    osmv::log::get_traceback_log() {
    return traceback_sink->storage;
}
