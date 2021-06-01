#include "resources.hpp"

#include "src/utils/os.hpp"
#include "src/log.hpp"

#include <fstream>
#include <filesystem>
#include <algorithm>
#include <sstream>
#include <vector>
#include <string>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string_view>

static std::vector<osc::Recent_file> load_recent_files_file(std::filesystem::path const& p) {
    std::vector<osc::Recent_file> rv;
    std::ifstream fd{p, std::ios::in};

    if (!fd) {
        // do not throw, because it probably shouldn't crash the application if this
        // is an issue
        osc::log::error("%s: could not be opened for reading: cannot load recent files list", p.string().c_str());
        return rv;
    }

    std::string line;
    while (std::getline(fd, line)) {
        std::istringstream ss{line};

        // read line content
        uint64_t timestamp;
        std::filesystem::path path;
        ss >> timestamp;
        ss >> path;

        // calc tertiary data
        bool exists = std::filesystem::exists(path);
        std::chrono::seconds timestamp_secs{timestamp};

        rv.emplace_back(exists, std::move(timestamp_secs), std::move(path));
    }

    return rv;
}

static std::chrono::seconds unix_timestamp() {
    return std::chrono::seconds(std::time(nullptr));
}

static std::filesystem::path recent_files_path() {
    return osc::user_data_dir() / "recent_files.txt";
}

void osc::find_files_with_extensions(
        std::filesystem::path const& root,
        std::string_view const* extensions,
        size_t n,
        std::vector<std::filesystem::path>& append_out) {

    if (!std::filesystem::exists(root)) {
        return;
    }

    if (!std::filesystem::is_directory(root)) {
        return;
    }

    for (std::filesystem::directory_entry const& e : std::filesystem::recursive_directory_iterator{root}) {
        for (size_t i = 0; i < n; ++i) {
            if (e.path().extension() == extensions[i]) {
                append_out.push_back(e.path());
            }
        }
    }
}

std::string osc::slurp(std::filesystem::path const& p) {
    std::ifstream f;
    f.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    f.open(p, std::ios::binary | std::ios::in);

    std::stringstream ss;
    ss << f.rdbuf();

    return std::move(ss).str();
}

std::vector<osc::Recent_file> osc::recent_files() {
    std::filesystem::path p = recent_files_path();

    if (!std::filesystem::exists(p)) {
        return {};
    }

    return load_recent_files_file(p);
}

void osc::add_recent_file(std::filesystem::path const& p) {
    std::filesystem::path rfs_path = recent_files_path();

    // load existing list
    std::vector<Recent_file> rfs;
    if (std::filesystem::exists(rfs_path)) {
        rfs = load_recent_files_file(rfs_path);
    }

    // clear potentially duplicate entries from existing list
    {
        auto it = std::remove_if(rfs.begin(), rfs.end(), [&p](Recent_file const& rf) { return rf.path == p; });
        rfs.erase(it, rfs.end());
    }

    // write by truncating existing list file
    std::ofstream fd{rfs_path, std::ios::trunc};

    if (!fd) {
        osc::log::error("%s: could not be opened for writing: cannot update recent files list", rfs_path.string().c_str());
    }

    // re-serialize the n newest entries (the loaded list is sorted oldest -> newest)
    auto begin = rfs.end() - (rfs.size() < 10 ? static_cast<int>(rfs.size()) : 10);
    for (auto it = begin; it != rfs.end(); ++it) {
        fd << it->last_opened_unix_timestamp.count() << ' ' << it->path << std::endl;
    }

    // append the new entry
    fd << unix_timestamp().count() << ' ' << std::filesystem::absolute(p) << std::endl;
}


