#include "Log.hpp"

#include <cstdio>
#include <exception>
#include <iostream>
#include <mutex>

using namespace osc;
using namespace osc::log;

namespace {
    class StdoutSink final : public Sink {
        std::mutex mutex;

        void log(LogMessage const& msg) override {
            std::lock_guard g{mutex};
            std::cout << '[' << msg.loggerName << "] [" << toStringView(msg.level) << "] " << msg.payload << std::endl;
        }
    };

    struct CircularLogSink final : public log::Sink {
        Mutex_guarded<CircularBuffer<OwnedLogMessage, g_MaxLogTracebackMessages>> storage;

        void log(log::LogMessage const& msg) override {
            auto l = storage.lock();
            l->emplace_back(msg);
        }
    };
}

static std::shared_ptr<Logger> createDefaultLogSink() {
    return std::make_shared<Logger>("default", std::make_shared<StdoutSink>());
}

static std::shared_ptr<CircularLogSink> createTracebackSink() {
    auto rv = std::make_shared<CircularLogSink>();
    log::defaultLoggerRaw()->sinks().push_back(rv);
    return rv;
}

static std::shared_ptr<Logger> g_DefaultLogSink = createDefaultLogSink();
static std::shared_ptr<CircularLogSink> g_TracebackSink = createTracebackSink();


// public API

std::string_view const osc::log::level::g_LogLevelStringViews[] OSC_LOG_LVL_NAMES;
char const* const osc::log::level::g_LogLevelCStrings[] OSC_LOG_LVL_NAMES;

std::shared_ptr<Logger> osc::log::defaultLogger() noexcept {
    return g_DefaultLogSink;
}

Logger* osc::log::defaultLoggerRaw() noexcept {
    return defaultLogger().get();
}

level::LevelEnum osc::log::getTracebackLevel() {
    return g_TracebackSink->level();
}

void osc::log::setTracebackLevel(level::LevelEnum lvl) {
    g_TracebackSink->set_level(lvl);
}

[[nodiscard]] Mutex_guarded<CircularBuffer<OwnedLogMessage, g_MaxLogTracebackMessages>>&
    osc::log::getTracebackLog() {
    return g_TracebackSink->storage;
}
