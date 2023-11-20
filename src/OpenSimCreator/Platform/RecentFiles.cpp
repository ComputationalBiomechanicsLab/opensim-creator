#include "RecentFiles.hpp"

#include <OpenSimCreator/Platform/RecentFile.hpp>

#include <oscar/Platform/App.hpp>
#include <oscar/Platform/Log.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <string>
#include <sstream>
#include <utility>
#include <vector>

namespace
{
    constexpr size_t c_MaxRecentFileEntries = 10;

    // returns a unix timestamp in seconds since the epoch
    std::chrono::seconds GetCurrentTimeAsUnixTimestamp()
    {
        return std::chrono::seconds(std::time(nullptr));
    }

    bool LastOpenedGreaterThan(osc::RecentFile const& a, osc::RecentFile const& b)
    {
        return a.lastOpenedUnixTimestamp > b.lastOpenedUnixTimestamp;
    }

    void SortNewestToOldest(std::vector<osc::RecentFile>& files)
    {
        std::sort(files.begin(), files.end(), LastOpenedGreaterThan);
    }

    // load the "recent files" file that the application persists to disk
    std::vector<osc::RecentFile> LoadRecentFilesFile(std::filesystem::path const& p)
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
            osc::log::error("%s: could not be opened for reading: cannot load recent files list", p.string().c_str());
            return {};
        }

        std::vector<osc::RecentFile> rv;
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
            bool exists = std::filesystem::exists(path);
            std::chrono::seconds timestampSecs{timestamp};

            rv.push_back(osc::RecentFile{exists, timestampSecs, std::move(path)});
        }

        SortNewestToOldest(rv);

        return rv;
    }

    // returns the filesystem path to the "recent files" file
    std::filesystem::path GetRecentFilesFilePath(std::filesystem::path const& userDataDirPath)
    {
        return userDataDirPath / "recent_files.txt";
    }
}

osc::RecentFiles::RecentFiles() :
    m_DiskLocation{GetRecentFilesFilePath(App::get().getUserDataDirPath())},
    m_Files{LoadRecentFilesFile(m_DiskLocation)}
{
}

void osc::RecentFiles::push_back(std::filesystem::path const& path)
{
    // remove duplicates
    std::erase_if(m_Files, [&path](RecentFile const& f)
    {
        return f.path == path;
    });

    m_Files.push_back(RecentFile
    {
        std::filesystem::exists(path),
        GetCurrentTimeAsUnixTimestamp(),
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
        osc::log::error("%s: could not be opened for writing: cannot update recent files list", m_DiskLocation.string().c_str());
        return;
    }

    // write up-to the first 10 entries
    size_t const numFilesToWrite = std::min(m_Files.size(), c_MaxRecentFileEntries);
    for (size_t i = 0; i < numFilesToWrite; ++i)
    {
        RecentFile const& rf = m_Files[i];
        fd << rf.lastOpenedUnixTimestamp.count() << ' ' << rf.path << '\n';
    }
}
