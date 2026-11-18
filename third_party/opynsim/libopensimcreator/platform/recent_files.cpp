#include "recent_files.h"

#include <libopensimcreator/platform/recent_file.h>

#include <liboscar/platform/app.h>
#include <liboscar/platform/log.h>
#include <liboscar/utilities/algorithms.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <functional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using namespace osc;
namespace rgs = std::ranges;

namespace
{
    constexpr size_t c_max_recent_file_entries = 10;

    // returns a unix timestamp in seconds since the epoch
    std::chrono::seconds get_current_time_as_unix_timestamp()
    {
        return std::chrono::seconds(std::time(nullptr));
    }

    void sort_newest_to_oldest(std::vector<RecentFile>& files)
    {
        rgs::sort(files, rgs::greater{}, [](const RecentFile& rf) {
            return rf.last_opened_unix_timestamp;
        });
    }

    // load the "recent files" file that the application persists to disk
    std::vector<RecentFile> load_recent_files_file(const std::filesystem::path& p)
    {
        if (!std::filesystem::exists(p)) {
            // the recent files txt does not exist (e.g. because it's the first time
            // that the user booted OSC - #786)
            return {};
        }

        std::ifstream fd{p, std::ios::in};

        if (!fd) {
            // do not throw, because it probably shouldn't crash the application if this
            // is an issue
            log_error(
                "{}: could not be opened for reading: cannot load recent files list", p.string());
            return {};
        }

        std::vector<RecentFile> rv;
        std::string line;

        while (std::getline(fd, line)) {
            std::istringstream ss{line};

            // read line content
            uint64_t timestamp = 0;
            std::filesystem::path path;
            ss >> timestamp;
            ss >> path;

            // calc tertiary data
            const bool exists = std::filesystem::exists(path);
            const std::chrono::seconds timestamp_secs{timestamp};

            rv.push_back(RecentFile{exists, timestamp_secs, std::move(path)});
        }

        sort_newest_to_oldest(rv);

        return rv;
    }

    // returns the filesystem path to the "recent files" file
    std::filesystem::path get_recent_files_file_path(
        const std::filesystem::path& user_data_dir_path)
    {
        return user_data_dir_path / "recent_files.txt";
    }
}

osc::RecentFiles::RecentFiles() :
    disk_location_{get_recent_files_file_path(App::get().user_data_directory())},
    files_{load_recent_files_file(disk_location_)}
{}

osc::RecentFiles::RecentFiles(std::filesystem::path recent_files_file) :
    disk_location_{std::move(recent_files_file)},
    files_{load_recent_files_file(disk_location_)}
{}

osc::RecentFiles::~RecentFiles() noexcept  // NOLINT(bugprone-exception-escape)
{
    try {
        sync();
    } catch (const std::exception& ex) {
        log_critical("Exception thrown when `sync()`ing `RecentFiles`: {}", ex.what());
    }
}

void osc::RecentFiles::push_back(const std::filesystem::path& path)
{
    // remove duplicates
    std::erase_if(files_, [&path](const RecentFile& f) { return f.path == path; });

    files_.push_back(
        RecentFile{
            std::filesystem::exists(path),
            get_current_time_as_unix_timestamp(),
            path,
        });

    ::sort_newest_to_oldest(files_);
}

void osc::RecentFiles::sync()
{
    // write by truncating existing list file
    std::ofstream fd{disk_location_, std::ios::trunc};
    if (!fd) {
        log_error(
            "{}: could not be opened for writing: cannot update recent files list",
            disk_location_.string());
        return;
    }

    // write up-to the first 10 entries
    const size_t num_files_to_write = min(files_.size(), c_max_recent_file_entries);
    for (size_t i = 0; i < num_files_to_write; ++i) {
        const RecentFile& rf = files_[i];
        fd << rf.last_opened_unix_timestamp.count() << ' ' << rf.path << '\n';
    }
}
