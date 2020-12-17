#include "globals.hpp"
#include <iostream>
#include <iomanip>

static osmv::clock::time_point g_app_startup_time = osmv::clock::now();

void osmv::DANGER_set_app_startup_time(clock::time_point tp) {
    g_app_startup_time = tp;
}

osmv::clock::time_point osmv::app_startup_time() {
    return g_app_startup_time;
}

void osmv::log_perf_bootup_event(const char* label) {
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - app_startup_time());
    std::cerr << "bootup_event @ ";
    std::cerr << std::setw(6) << millis.count();
    std::cerr << ": " << label << std::endl;
}
