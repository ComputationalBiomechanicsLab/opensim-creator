#include "Log.hpp"

#include <iostream>
#include <mutex>

namespace
{
    class StdoutSink final : public osc::log::Sink {
        std::mutex mutex;

        void log(osc::log::LogMessage const& msg) override
        {
            std::lock_guard g{mutex};
            std::cerr << '[' << msg.loggerName << "] [" << osc::log::toStringView(msg.level) << "] " << msg.payload << std::endl;
        }
    };

    struct CircularLogSink final : public osc::log::Sink {
        osc::SynchronizedValue<CircularBuffer<osc::log::OwnedLogMessage, osc::log::g_MaxLogTracebackMessages>> storage;

        void log(osc::log::LogMessage const& msg) override
        {
            auto l = storage.lock();
            l->emplace_back(msg);
        }
    };
}

static std::shared_ptr<osc::log::Logger> createDefaultLogSink()
{
    return std::make_shared<osc::log::Logger>("default", std::make_shared<StdoutSink>());
}

static std::shared_ptr<CircularLogSink> createTracebackSink()
{
    auto rv = std::make_shared<CircularLogSink>();
    osc::log::defaultLoggerRaw()->sinks().push_back(rv);
    return rv;
}

static std::shared_ptr<osc::log::Logger> g_DefaultLogSink = createDefaultLogSink();
static std::shared_ptr<CircularLogSink> g_TracebackSink = createTracebackSink();


// public API

std::string_view const osc::log::level::g_LogLevelStringViews[] OSC_LOG_LVL_NAMES;
char const* const osc::log::level::g_LogLevelCStrings[] OSC_LOG_LVL_NAMES;

std::shared_ptr<osc::log::Logger> osc::log::defaultLogger() noexcept
{
    return g_DefaultLogSink;
}

osc::log::Logger* osc::log::defaultLoggerRaw() noexcept
{
    return g_DefaultLogSink.get();
}

osc::log::level::LevelEnum osc::log::getTracebackLevel()
{
    return g_TracebackSink->level();
}

void osc::log::setTracebackLevel(level::LevelEnum lvl)
{
    g_TracebackSink->set_level(lvl);
}

osc::SynchronizedValue<CircularBuffer<osc::log::OwnedLogMessage, osc::log::g_MaxLogTracebackMessages>>& osc::log::getTracebackLog()
{
    return g_TracebackSink->storage;
}
