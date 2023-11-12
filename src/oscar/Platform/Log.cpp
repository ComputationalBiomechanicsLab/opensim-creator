#include "Log.hpp"

#include <oscar/Platform/Log.hpp>
#include <oscar/Utils/CStringView.hpp>

#include <cstddef>
#include <iostream>
#include <mutex>

namespace
{
    class StdoutSink final : public osc::LogSink {
    private:
        void implLog(osc::LogMessageView const& msg) final
        {
            static std::mutex s_StdoutMutex;

            std::lock_guard g{s_StdoutMutex};
            std::cerr << '[' << msg.loggerName << "] [" << osc::ToCStringView(msg.level) << "] " << msg.payload << std::endl;
        }
    };

    class CircularLogSink final : public osc::LogSink {
    public:
        osc::SynchronizedValue<osc::CircularBuffer<osc::LogMessage, osc::log::c_MaxLogTracebackMessages>>& updMessages()
        {
            return m_Messages;
        }
    private:
        void implLog(osc::LogMessageView const& msg) final
        {
            auto l = m_Messages.lock();
            l->emplace_back(msg);
        }

        osc::SynchronizedValue<osc::CircularBuffer<osc::LogMessage, osc::log::c_MaxLogTracebackMessages>> m_Messages;
    };

    struct GlobalSinks final {
        GlobalSinks() :
            defaultLogSink{std::make_shared<osc::Logger>("default", std::make_shared<StdoutSink>())},
            tracebackSink{std::make_shared<CircularLogSink>()}
        {
            defaultLogSink->sinks().push_back(tracebackSink);
        }

        std::shared_ptr<osc::Logger> defaultLogSink;
        std::shared_ptr<CircularLogSink> tracebackSink;
    };

    GlobalSinks& GetGlobalSinks()
    {
        static GlobalSinks s_GlobalSinks;
        return s_GlobalSinks;
    }
}

// public API

std::shared_ptr<osc::Logger> osc::log::defaultLogger() noexcept
{
    return GetGlobalSinks().defaultLogSink;
}

osc::Logger* osc::log::defaultLoggerRaw() noexcept
{
    return GetGlobalSinks().defaultLogSink.get();
}

osc::LogLevel osc::log::getTracebackLevel()
{
    return GetGlobalSinks().tracebackSink->getLevel();
}

void osc::log::setTracebackLevel(LogLevel lvl)
{
    GetGlobalSinks().tracebackSink->setLevel(lvl);
}

osc::SynchronizedValue<osc::CircularBuffer<osc::LogMessage, osc::log::c_MaxLogTracebackMessages>>& osc::log::getTracebackLog()
{
    return GetGlobalSinks().tracebackSink->updMessages();
}
