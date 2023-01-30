#include "Log.hpp"

#include "src/Utils/Algorithms.hpp"
#include "src/Utils/CStringView.hpp"

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
        osc::SynchronizedValue<osc::CircularBuffer<osc::log::OwnedLogMessage, osc::log::c_MaxLogTracebackMessages>> storage;

        void log(osc::log::LogMessage const& msg) override
        {
            auto l = storage.lock();
            l->emplace_back(msg);
        }
    };

    struct GlobalSinks final {
        GlobalSinks() :
            defaultLogSink{std::make_shared<osc::log::Logger>("default", std::make_shared<StdoutSink>())},
            tracebackSink{std::make_shared<CircularLogSink>()}
        {
            defaultLogSink->sinks().push_back(tracebackSink);
        }

        std::shared_ptr<osc::log::Logger> defaultLogSink;
        std::shared_ptr<CircularLogSink> tracebackSink;
    };

    GlobalSinks& GetGlobalSinks()
    {
        static GlobalSinks s_GlobalSinks;
        return s_GlobalSinks;
    }

    auto constexpr c_LogLevelStrings = osc::MakeSizedArray<osc::CStringView, static_cast<size_t>(osc::log::level::LevelEnum::NUM_LEVELS)>
    (
        "trace",
        "debug",
        "info",
        "warning",
        "error",
        "critical",
        "off"
    );
}

// public API

std::string_view osc::log::toStringView(level::LevelEnum level)
{
    return c_LogLevelStrings[level];
}

char const* osc::log::toCStr(level::LevelEnum level)
{
    return c_LogLevelStrings[level].c_str();
}

std::shared_ptr<osc::log::Logger> osc::log::defaultLogger() noexcept
{
    return GetGlobalSinks().defaultLogSink;
}

osc::log::Logger* osc::log::defaultLoggerRaw() noexcept
{
    return GetGlobalSinks().defaultLogSink.get();
}

osc::log::level::LevelEnum osc::log::getTracebackLevel()
{
    return GetGlobalSinks().tracebackSink->level();
}

void osc::log::setTracebackLevel(level::LevelEnum lvl)
{
    GetGlobalSinks().tracebackSink->set_level(lvl);
}

osc::SynchronizedValue<osc::CircularBuffer<osc::log::OwnedLogMessage, osc::log::c_MaxLogTracebackMessages>>& osc::log::getTracebackLog()
{
    return GetGlobalSinks().tracebackSink->storage;
}
