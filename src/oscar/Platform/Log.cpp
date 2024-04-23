#include "Log.h"

#include <oscar/Utils/CStringView.h>

#include <iostream>
#include <mutex>

namespace detail = osc::detail;
using namespace osc;

namespace
{
    class StdoutSink final : public LogSink {
    private:
        void impl_log_message(const LogMessageView& message) final
        {
            static std::mutex s_stdout_mutex;

            const std::lock_guard g{s_stdout_mutex};
            std::cerr << '[' << message.logger_name() << "] [" << message.level() << "] " << message.payload() << std::endl;
        }
    };

    class CircularLogSink final : public LogSink {
    public:
        SynchronizedValue<CircularBuffer<LogMessage, detail::c_max_log_traceback_messages>>& upd_messages()
        {
            return messages_;
        }

    private:
        void impl_log_message(const LogMessageView& msg) final
        {
            auto l = messages_.lock();
            l->emplace_back(msg);
        }

        SynchronizedValue<CircularBuffer<LogMessage, detail::c_max_log_traceback_messages>> messages_;
    };

    struct GlobalSinks final {
        GlobalSinks()
        {
            default_log_sink->sinks().push_back(traceback_sink);
        }

        std::shared_ptr<Logger> default_log_sink = std::make_shared<Logger>("default", std::make_shared<StdoutSink>());
        std::shared_ptr<CircularLogSink> traceback_sink = std::make_shared<CircularLogSink>();
    };

    GlobalSinks& get_global_sinks()
    {
        static GlobalSinks s_global_sinks;
        return s_global_sinks;
    }
}

std::shared_ptr<Logger> osc::global_default_logger()
{
    return get_global_sinks().default_log_sink;
}

Logger* osc::global_default_logger_raw()
{
    return get_global_sinks().default_log_sink.get();
}

LogLevel osc::global_get_traceback_level()
{
    return get_global_sinks().traceback_sink->level();
}

void osc::global_set_traceback_level(LogLevel lvl)
{
    get_global_sinks().traceback_sink->set_level(lvl);
}

SynchronizedValue<CircularBuffer<LogMessage, detail::c_max_log_traceback_messages>>& osc::global_get_traceback_log()
{
    return get_global_sinks().traceback_sink->upd_messages();
}
