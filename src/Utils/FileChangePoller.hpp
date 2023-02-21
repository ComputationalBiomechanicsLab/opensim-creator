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
            std::string const& path
        );

        bool changeWasDetected(std::string const& path);

    private:
        std::chrono::milliseconds m_DelayBetweenChecks;
        std::chrono::system_clock::time_point m_NextPollingTime;
        std::filesystem::file_time_type m_FileLastModificationTime;
        bool m_IsEnabled;
    };
}
