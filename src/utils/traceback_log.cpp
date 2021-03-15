#include "traceback_log.hpp"

#include <memory>

using namespace osmv;

namespace {
    struct Circular_log_sink final : public log::Sink {
        Mutex_guarded<Circular_buffer<Owned_log_msg, max_traceback_log_messages>> storage;

        void log(log::Log_msg const& msg) override {
            auto l = storage.lock();
            l->emplace_back(msg);
        }
    };
}

static std::shared_ptr<Circular_log_sink> create_default_sink() {
    auto rv = std::make_shared<Circular_log_sink>();
    log::default_logger_raw()->sinks().push_back(rv);
    return rv;
}

void osmv::init_traceback_log() {
    (void)get_traceback_log();
}

Mutex_guarded<Circular_buffer<Owned_log_msg, max_traceback_log_messages>>& osmv::get_traceback_log() {
    static std::shared_ptr<Circular_log_sink> sink = create_default_sink();
    return sink->storage;
}
