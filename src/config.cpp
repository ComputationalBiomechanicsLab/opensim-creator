#include "config.hpp"

#include "src/utils/os.hpp"
#include "osc_build_config.hpp"

#include <toml.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace fs = std::filesystem;

struct App_config final {
    std::filesystem::path resource_dir;
    bool use_multi_viewport;
};

// config defaults when a config file/property isn't present

static App_config load_application_config() {
    // the "system-wide" application config is searched recursively by stepping
    // up the directory tree, searching for `osc.toml`.
    //
    // If it is found, then the values in that file are used. If not, then use
    // reasonable defaults.
    //
    // note: for development, a config file is generated which hard-codes the
    //       absolute path to the developer's resource dir into the config file
    //       so that devs don't have to copy things around while developing

    fs::path p = osc::current_exe_dir();
    bool exists = false;
    while (p.has_filename()) {
        fs::path maybe_config = p / "osc.toml";
        if (fs::exists(maybe_config)) {
            p = maybe_config;
            exists = true;
            break;
        }
        // HACK: there is a file at "MacOS/osc.toml", which is where the config
        // is relative to SDL_GetBasePath. current_exe_dir should be fixed
        // accordingly.
        fs::path maybe_macos_config = p / "MacOS" / "osc.toml";
        if (fs::exists(maybe_macos_config)) {
            p = maybe_macos_config;
            exists = true;
            break;
        }
        p = p.parent_path();
    }

    // no config: return an in-memory config that has reasonable defaults
    if (!exists) {
        return App_config{OSC_DEFAULT_RESOURCE_DIR, OSC_DEFAULT_USE_MULTI_VIEWPORT};
    }

    // warning: can throw
    auto config = toml::parse_file(p.c_str());
    std::string_view resource_dir = config["resources"].value_or(OSC_DEFAULT_RESOURCE_DIR);

    // configuration resource_dir is relative *to the configuration file*
    fs::path config_file_dir = p.parent_path();
    fs::path resource_dir_path = config_file_dir / resource_dir;

    bool use_multi_viewport = config["experimental_feature_flags"]["multiple_viewports"].value_or(OSC_DEFAULT_USE_MULTI_VIEWPORT);

    return App_config{resource_dir_path, use_multi_viewport};
}

static App_config load_config() {
    static const App_config config = load_application_config();
    return config;
}

std::filesystem::path osc::config::resource_path(std::filesystem::path const& subpath) {
    return load_config().resource_dir / subpath;
}

static std::filesystem::path const shaders_dir = "shaders";
std::filesystem::path osc::config::shader_path(char const* shader_name) {
    return resource_path(shaders_dir / shader_name);
}

static fs::path get_recent_files_path() {
    return osc::user_data_dir() / "recent_files.txt";
}

static std::vector<osc::config::Recent_file> load_recent_files_file(std::filesystem::path const& p) {
    std::ifstream fd{p, std::ios::in};

    if (!fd) {
        std::stringstream ss;
        ss << p;
        ss << ": could not be opened for reading: cannot load recent files list";
        throw std::runtime_error{std::move(ss).str()};
    }

    std::vector<osc::config::Recent_file> rv;
    std::string line;
    while (std::getline(fd, line)) {
        std::istringstream ss{line};

        // read line content
        uint64_t timestamp;
        fs::path path;
        ss >> timestamp;
        ss >> path;

        // calc tertiary data
        bool exists = fs::exists(path);
        std::chrono::seconds timestamp_secs{timestamp};

        rv.emplace_back(exists, std::move(timestamp_secs), std::move(path));
    }

    return rv;
}

static std::chrono::seconds unix_timestamp() {
    return std::chrono::seconds(std::time(nullptr));
}

std::vector<osc::config::Recent_file> osc::config::recent_files() {
    fs::path recent_files_path = get_recent_files_path();

    if (!fs::exists(recent_files_path)) {
        return {};
    }

    return load_recent_files_file(recent_files_path);
}

void osc::config::add_recent_file(std::filesystem::path const& p) {
    fs::path rfs_path = get_recent_files_path();

    // load existing list
    std::vector<Recent_file> rfs;
    if (fs::exists(rfs_path)) {
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
        std::stringstream ss;
        ss << rfs_path;
        ss << ": could not be opened for writing: cannot update recent files list";
        throw std::runtime_error{std::move(ss).str()};
    }

    // re-serialize the n newest entries (the loaded list is sorted oldest -> newest)
    auto begin = rfs.end() - (rfs.size() < 10 ? static_cast<int>(rfs.size()) : 10);
    for (auto it = begin; it != rfs.end(); ++it) {
        fd << it->last_opened_unix_timestamp.count() << ' ' << it->path << std::endl;
    }

    // append the new entry
    fd << unix_timestamp().count() << ' ' << fs::absolute(p) << std::endl;
}

bool osc::config::should_use_multi_viewport() {
    return load_config().use_multi_viewport;
}

static bool filename_lexographically_gt(fs::path const& a, fs::path const& b) {
    return a.filename() < b.filename();
}

std::vector<fs::path> osc::config::example_osim_files() {
    fs::path models_dir = resource_path("models");

    std::vector<fs::path> rv;

    if (!fs::exists(models_dir)) {
        // probably running from a weird location, or resources are missing
        return rv;
    }

    if (!fs::is_directory(models_dir)) {
        // something horrible has happened, but gracefully fallback to ignoring
        // that issue (grumble grumble, this should be logged)
        return rv;
    }

    for (fs::directory_entry const& e : fs::recursive_directory_iterator{models_dir}) {
        if (e.path().extension() == ".osim") {
            rv.push_back(e.path());
        }
    }

    std::sort(rv.begin(), rv.end(), filename_lexographically_gt);

    return rv;
}
