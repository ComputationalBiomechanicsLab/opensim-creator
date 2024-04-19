#pragma once

#include <chrono>
#include <filesystem>
#include <string>

namespace osc
{
    class FileChangePoller final {
    public:
        FileChangePoller(
            std::chrono::milliseconds delayBetweenChecks,
            const std::string& path
        );

        bool changeWasDetected(const std::string& path);

    private:
        std::chrono::milliseconds m_DelayBetweenChecks;
        std::chrono::system_clock::time_point m_NextPollingTime;
        std::filesystem::file_time_type m_FileLastModificationTime;
        bool m_IsEnabled;
    };
}
