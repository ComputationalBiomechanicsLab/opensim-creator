#include "FileChangePoller.hpp"

static constexpr char const* g_ModelNoBackingFileSenteniel = "Unassigned";

std::filesystem::file_time_type GetLastModificationTime(std::string const& path)
{
    if (path.empty() || path == g_ModelNoBackingFileSenteniel)
    {
        return std::filesystem::file_time_type{};
    }
    else
    {
        return std::filesystem::last_write_time(path);
    }
}

osc::FileChangePoller::FileChangePoller(
        std::chrono::milliseconds delay,
        std::string const& path) :

    m_DelayBetweenChecks{delay},
    m_NextPollingTime{std::chrono::system_clock::now() + delay},
    m_FileLastModificationTime{GetLastModificationTime(path)},
    m_IsEnabled{true}
{
}

bool osc::FileChangePoller::changeWasDetected(std::string const& path)
{
    if (!m_IsEnabled)
    {
        return false;
    }

    if (path.empty() || path == g_ModelNoBackingFileSenteniel)
    {
        return false;
    }

    auto now = std::chrono::system_clock::now();

    if (now < m_NextPollingTime)
    {
        return false;
    }

    auto modification_time = std::filesystem::last_write_time(path);
    m_NextPollingTime = now + m_DelayBetweenChecks;

    if (modification_time == m_FileLastModificationTime)
    {
        return false;
    }

    m_FileLastModificationTime = modification_time;

    return true;
}
