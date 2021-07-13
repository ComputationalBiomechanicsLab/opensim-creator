#include "log.hpp"

#include <cstdio>
#include <exception>
#include <iostream>
#include <mutex>

using namespace osc;
using namespace osc::log;

namespace {
    class Stdout_sink final : public Sink {
        std::mutex mutex;

        void log(Log_msg const& msg) override {
            std::lock_guard g{mutex};
            std::cout << '[' << msg.logger_name << "] [" << to_string_view(msg.level) << "] " << msg.payload << std::endl;
        }
    };

    struct Circular_log_sink final : public log::Sink {
        Mutex_guarded<Circular_buffer<Owned_log_msg, g_MaxLogTracebackMessages>> storage;

        void log(log::Log_msg const& msg) override {
            auto l = storage.lock();
            l->emplace_back(msg);
        }
    };

    std::shared_ptr<Logger> create_default_sink() {
        return std::make_shared<Logger>("default", std::make_shared<Stdout_sink>());
    }

    std::shared_ptr<Circular_log_sink> create_traceback_sink() {
        auto rv = std::make_shared<Circular_log_sink>();
        log::default_logger_raw()->sinks().push_back(rv);
        return rv;
    }

    std::shared_ptr<Logger> g_DefaultLogSink = create_default_sink();
    std::shared_ptr<Circular_log_sink> g_TracebackSink = create_traceback_sink();
}


// public API

std::string_view const osc::log::level::g_LogLevelStringViews[] OSC_LOG_LVL_NAMES;
char const* const osc::log::level::g_LogLevelCStrings[] OSC_LOG_LVL_NAMES;

std::shared_ptr<Logger> osc::log::default_logger() noexcept {
    return g_DefaultLogSink;
}

Logger* osc::log::default_logger_raw() noexcept {
    return default_logger().get();
}

level::Level_enum osc::log::get_traceback_level() {
    return g_TracebackSink->level();
}

void osc::log::set_traceback_level(level::Level_enum lvl) {
    g_TracebackSink->set_level(lvl);
}

[[nodiscard]] Mutex_guarded<Circular_buffer<Owned_log_msg, g_MaxLogTracebackMessages>>&
    osc::log::get_traceback_log() {
    return g_TracebackSink->storage;
}
