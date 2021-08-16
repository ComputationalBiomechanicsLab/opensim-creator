#pragma once

#include <chrono>

namespace osc {
    struct Timer_guard final {
        std::chrono::high_resolution_clock::duration& out;
        std::chrono::high_resolution_clock::time_point p;

        Timer_guard(std::chrono::high_resolution_clock::duration& out_) :
            out{out_},
            p{std::chrono::high_resolution_clock::now()} {
        }

        ~Timer_guard() noexcept {
            out = std::chrono::high_resolution_clock::now() - p;
        }
    };

    struct Basic_perf_timer final {
        std::chrono::high_resolution_clock::duration val{0};

        Timer_guard measure() {
            return Timer_guard{val};
        }

        float micros() {
            auto us = std::chrono::duration_cast<std::chrono::microseconds>(val);
            return static_cast<float>(us.count());
        }

        float millis() {
            return micros()/1000.0f;
        }

        float secs() {
            return micros()/1000000.0f;
        }
    };
}
