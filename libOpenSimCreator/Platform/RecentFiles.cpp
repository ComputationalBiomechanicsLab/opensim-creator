#include "RecentFiles.h"

#include <libOpenSimCreator/Platform/RecentFile.h>

#include <liboscar/Platform/App.h>
#include <liboscar/Platform/Log.h>
#include <liboscar/Utils/Algorithms.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <ranges>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using namespace osc;
namespace rgs = std::ranges;

namespace
{
    constexpr size_t c_MaxRecentFileEntries = 10;

    // returns a unix timestamp in seconds since the epoch
    std::chrono::seconds get_current_time_as_unix_timestamp()
    {
        return std::chrono::seconds(std::time(nullptr));
    }

    void SortNewestToOldest(std::vector<RecentFile>& files)
    {
        rgs::sort(files, rgs::greater{}, [](const RecentFile& rf) { return rf.lastOpenedUnixTimestamp; });
    }

    // load the "recent files" file that the application persists to disk
    std::vector<RecentFile> LoadRecentFilesFile(const std::filesystem::path& p)
    {
        if (!std::filesystem::exists(p))
        {
            // the recent files txt does not exist (e.g. because it's the first time
            // that the user booted OSC - #786)
            return {};
        }

        std::ifstream fd{p, std::ios::in};

        if (!fd)
        {
            // do not throw, because it probably shouldn't crash the application if this
            // is an issue
            log_error("%s: could not be opened for reading: cannot load recent files list", p.string().c_str());
            return {};
        }

        std::vector<RecentFile> rv;
        std::string line;

        while (std::getline(fd, line))
        {
            std::istringstream ss{line};

            // read line content
            uint64_t timestamp = 0;
            std::filesystem::path path;
            ss >> timestamp;
            ss >> path;

            // calc tertiary data
            const bool exists = std::filesystem::exists(path);
            const std::chrono::seconds timestampSecs{timestamp};

            rv.push_back(RecentFile{exists, timestampSecs, std::move(path)});
        }

        SortNewestToOldest(rv);

        return rv;
    }

    // returns the filesystem path to the "recent files" file
    std::filesystem::path GetRecentFilesFilePath(const std::filesystem::path& userDataDirPath)
    {
        return userDataDirPath / "recent_files.txt";
    }
}

osc::RecentFiles::RecentFiles() :
    m_DiskLocation{GetRecentFilesFilePath(App::get().user_data_directory())},
    m_Files{LoadRecentFilesFile(m_DiskLocation)}
{
}

osc::RecentFiles::RecentFiles(std::filesystem::path recentFilesFile) :
    m_DiskLocation{std::move(recentFilesFile)},
    m_Files{LoadRecentFilesFile(m_DiskLocation)}
{
}

void osc::RecentFiles::push_back(const std::filesystem::path& path)
{
    // remove duplicates
    std::erase_if(m_Files, [&path](const RecentFile& f)
    {
        return f.path == path;
    });

    m_Files.push_back(RecentFile
    {
        std::filesystem::exists(path),
        get_current_time_as_unix_timestamp(),
        path,
    });

    ::SortNewestToOldest(m_Files);
}

void osc::RecentFiles::sync()
{
    // write by truncating existing list file
    std::ofstream fd{m_DiskLocation, std::ios::trunc};
    if (!fd)
    {
        log_error("%s: could not be opened for writing: cannot update recent files list", m_DiskLocation.string().c_str());
        return;
    }

    // write up-to the first 10 entries
    const size_t numFilesToWrite = min(m_Files.size(), c_MaxRecentFileEntries);
    for (size_t i = 0; i < numFilesToWrite; ++i)
    {
        const RecentFile& rf = m_Files[i];
        fd << rf.lastOpenedUnixTimestamp.count() << ' ' << rf.path << '\n';
    }
}
