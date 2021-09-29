#pragma once

#include <chrono>

namespace osc {

    struct TimerGuard final {
        std::chrono::high_resolution_clock::duration& out;
        std::chrono::high_resolution_clock::time_point p;
        bool stopped;

        TimerGuard(std::chrono::high_resolution_clock::duration& out_) :
            out{out_},
            p{std::chrono::high_resolution_clock::now()},
            stopped{false}
        {
        }

        void stop() {
            if (!stopped) {
                out = std::chrono::high_resolution_clock::now() - p;
                stopped = true;
            }
        }

        ~TimerGuard() noexcept {
            stop();
        }
    };

    struct BasicPerfTimer final {
        std::chrono::high_resolution_clock::duration val{0};

        TimerGuard measure() {
            return TimerGuard{val};
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
