#pragma once

#include <chrono>
#include <filesystem>

namespace osmv {

    struct File_change_poller final {
        using clock = std::chrono::system_clock;

        std::filesystem::path path;
        clock::time_point last_timepoint;
        std::chrono::milliseconds delay;
        clock::time_point next;

        // assumes that the caller wants to monitor changes *after* this constructor
        // is called
        File_change_poller(std::filesystem::path _path, std::chrono::milliseconds _delay) :
            path{std::move(_path)},
            last_timepoint{std::filesystem::last_write_time(path)},
            delay{std::move(_delay)},
            next{clock::now() + delay} {
        }

        bool change_detected() {
            auto now = clock::now();

            if (now < next) {
                return false;
            }

            auto tp = std::filesystem::last_write_time(path);
            next = now + delay;

            if (tp == last_timepoint) {
                return false;
            }

            last_timepoint = tp;
            return true;
        }
    };
}
