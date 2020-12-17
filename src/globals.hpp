#pragma once

#include <chrono>

namespace osmv {
    using clock = std::chrono::steady_clock;

    // so that `main` can call it
    //
    // defaults to a statically-initialized clock if not called
    void DANGER_set_app_startup_time(clock::time_point tp);

    clock::time_point app_startup_time();

    void log_perf_bootup_event(const char* label);
}
