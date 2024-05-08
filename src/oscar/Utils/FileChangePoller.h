#pragma once

#include <chrono>
#include <filesystem>
#include <string>

namespace osc
{
    class FileChangePoller final {
    public:
        FileChangePoller(
            std::chrono::milliseconds delay_between_checks,
            const std::string& path
        );

        bool change_detected(const std::string& path);

    private:
        std::chrono::milliseconds delay_between_checks_;
        std::chrono::system_clock::time_point next_polling_time_;
        std::filesystem::file_time_type file_last_modification_time_;
        bool enabled_;
    };
}
