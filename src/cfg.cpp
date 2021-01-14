#include "cfg.hpp"

#include "os.hpp"

#include <toml.hpp>

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

struct App_config final {
    std::filesystem::path resource_dir;
};

static App_config load_application_config() {
    // the "system-wide" application config is searched recursively by stepping
    // up the directory tree, searching for `osmv.toml`.
    //
    // If it is found, then the values in that file are used. If not, then use
    // reasonable defaults.
    //
    // note: for development, a config file is generated which hard-codes the
    //       absolute path to the developer's resource dir into the config file
    //       so that devs don't have to copy things around while developing

    fs::path p = osmv::current_exe_dir();
    bool exists = false;
    while (p.has_filename()) {
        fs::path maybe_config = p / "osmv.toml";
        if (fs::exists(maybe_config)) {
            p = maybe_config;
            exists = true;
            break;
        }
        p = p.parent_path();
    }

    // no config: return an in-memory config that has reasonable defaults
    if (not exists) {
        fs::path default_resource_dir = fs::path{".."} / "resources";
        return App_config{default_resource_dir};
    }

    // warning: can throw
    auto config = toml::parse_file(p.c_str());
    std::string_view resource_dir = config["resources"].value_or("../resources");

    // configuration resource_dir is relative *to the configuration file*
    fs::path config_file_dir = p.parent_path();
    fs::path resource_dir_path = config_file_dir / resource_dir;

    return App_config{resource_dir_path};
}

static App_config load_config() {
    static const App_config config = load_application_config();
    return config;
}

std::filesystem::path osmv::cfg::resource_path(std::filesystem::path const& subpath) {
    return load_config().resource_dir / subpath;
}

static std::filesystem::path const shaders_dir = "shaders";
std::filesystem::path osmv::cfg::shader_path(char const* shader_name) {
    return resource_path(shaders_dir / shader_name);
}
