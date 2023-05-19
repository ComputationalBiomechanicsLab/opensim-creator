#include "Log.hpp"

#include "oscar/Utils/Algorithms.hpp"
#include "oscar/Utils/CStringView.hpp"

#include <iostream>
#include <mutex>

namespace
{
    class StdoutSink final : public osc::log::Sink {
    private:
        void implLog(osc::log::LogMessage const& msg) final
        {
            static std::mutex s_StdoutMutex;

            std::lock_guard g{s_StdoutMutex};
            std::cerr << '[' << msg.loggerName << "] [" << osc::log::toCStringView(msg.level) << "] " << msg.payload << std::endl;
        }
    };

    class CircularLogSink final : public osc::log::Sink {
    public:
        osc::SynchronizedValue<osc::CircularBuffer<osc::log::OwnedLogMessage, osc::log::c_MaxLogTracebackMessages>>& updMessages()
        {
            return m_Messages;
        }
    private:
        void implLog(osc::log::LogMessage const& msg) final
        {
            auto l = m_Messages.lock();
            l->emplace_back(msg);
        }

        osc::SynchronizedValue<osc::CircularBuffer<osc::log::OwnedLogMessage, osc::log::c_MaxLogTracebackMessages>> m_Messages;
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

osc::CStringView osc::log::toCStringView(level::LevelEnum level)
{
    return c_LogLevelStrings.at(level);
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
    return GetGlobalSinks().tracebackSink->updMessages();
}
