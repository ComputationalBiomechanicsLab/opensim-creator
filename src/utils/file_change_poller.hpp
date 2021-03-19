#pragma once

#include <chrono>
#include <filesystem>
#include <functional>

namespace osmv {
    struct File_change_poller final {
        using clock = std::chrono::system_clock;

        std::chrono::milliseconds delay;
        clock::time_point next;
        std::filesystem::file_time_type last_modification_time;
        bool enabled;

        File_change_poller(std::chrono::milliseconds _delay, std::string const& path) :
            delay{_delay},
            next{clock::now() + delay},
            last_modification_time{path.empty() || path == "Unassigned" ? std::filesystem::file_time_type{}
                                                                        : std::filesystem::last_write_time(path)},
            enabled{true} {
        }

        bool change_detected(std::string const& path) {
            if (!enabled) {
                return false;
            }

            if (path.empty() || path == "Unassigned") {
                return false;
            }

            auto now = clock::now();

            if (now < next) {
                return false;
            }

            auto modification_time = std::filesystem::last_write_time(path);
            next = now + delay;

            if (modification_time == last_modification_time) {
                return false;
            }

            last_modification_time = modification_time;
            return true;
        }
    };
}
