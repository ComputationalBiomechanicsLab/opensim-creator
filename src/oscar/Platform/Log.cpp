#include "Log.hpp"

#include <oscar/Platform/Log.hpp>
#include <oscar/Utils/CStringView.hpp>

#include <cstddef>
#include <iostream>
#include <mutex>

using osc::CircularBuffer;
using osc::Logger;
using osc::LogLevel;
using osc::LogMessage;
using osc::LogMessageView;
using osc::LogSink;
using osc::SynchronizedValue;

namespace
{
    class StdoutSink final : public LogSink {
    private:
        void implLog(LogMessageView const& msg) final
        {
            static std::mutex s_StdoutMutex;

            std::lock_guard g{s_StdoutMutex};
            std::cerr << '[' << msg.loggerName << "] [" << ToCStringView(msg.level) << "] " << msg.payload << std::endl;
        }
    };

    class CircularLogSink final : public LogSink {
    public:
        SynchronizedValue<CircularBuffer<LogMessage, osc::log::c_MaxLogTracebackMessages>>& updMessages()
        {
            return m_Messages;
        }
    private:
        void implLog(LogMessageView const& msg) final
        {
            auto l = m_Messages.lock();
            l->emplace_back(msg);
        }

        SynchronizedValue<CircularBuffer<LogMessage, osc::log::c_MaxLogTracebackMessages>> m_Messages;
    };

    struct GlobalSinks final {
        GlobalSinks() :
            defaultLogSink{std::make_shared<Logger>("default", std::make_shared<StdoutSink>())},
            tracebackSink{std::make_shared<CircularLogSink>()}
        {
            defaultLogSink->sinks().push_back(tracebackSink);
        }

        std::shared_ptr<Logger> defaultLogSink;
        std::shared_ptr<CircularLogSink> tracebackSink;
    };

    GlobalSinks& GetGlobalSinks()
    {
        static GlobalSinks s_GlobalSinks;
        return s_GlobalSinks;
    }
}

// public API

std::shared_ptr<Logger> osc::log::defaultLogger()
{
    return GetGlobalSinks().defaultLogSink;
}

Logger* osc::log::defaultLoggerRaw()
{
    return GetGlobalSinks().defaultLogSink.get();
}

LogLevel osc::log::getTracebackLevel()
{
    return GetGlobalSinks().tracebackSink->getLevel();
}

void osc::log::setTracebackLevel(LogLevel lvl)
{
    GetGlobalSinks().tracebackSink->setLevel(lvl);
}

SynchronizedValue<CircularBuffer<LogMessage, osc::log::c_MaxLogTracebackMessages>>& osc::log::getTracebackLog()
{
    return GetGlobalSinks().tracebackSink->updMessages();
}
