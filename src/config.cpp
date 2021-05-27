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
#include <optional>

namespace fs = std::filesystem;

// static init with compiled-in defaults
//
// this handles the edge-case where the user boots the application without
// a config file
std::unique_ptr<osc::Config> osc::g_ApplicationConfig = std::make_unique<osc::Config>();

osc::Config::Config() :
    resource_dir{OSC_DEFAULT_RESOURCE_DIR},
    use_multi_viewport{OSC_DEFAULT_USE_MULTI_VIEWPORT} {
}

static std::optional<std::filesystem::path> try_locate_system_config() {
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

    if (exists) {
        return p;
    } else {
        return std::nullopt;
    }
}

void osc::init_load_config() {
    std::optional<std::filesystem::path> config_pth = try_locate_system_config();

    if (!config_pth) {
        log::info("could not find a system configuration file: OSC will still work, but might be missing some configured behavior");
        return;
    }

    toml::v2::table config;
    try {
        config = toml::parse_file(config_pth->c_str());
    } catch (std::exception const& ex) {
        log::error("error parsing config toml: %s", ex.what());
        log::error("OSC will continue to boot, but you might need to fix your config file (e.g. by deleting it)");
        return;
    }

    // init `resource_dir`
    {
        auto maybe_resources = config["resources"];
        if (maybe_resources) {
            std::string rp = (*maybe_resources.as_string()).get();

            // configuration resource_dir is relative *to the configuration file*
            fs::path config_file_dir = config_pth->parent_path();
            g_ApplicationConfig->resource_dir.value = config_file_dir / rp;
        }
    }

    // init `use_multi_viewport`
    {
        auto maybe_usemvp = config["experimental_feature_flags"]["multiple_viewports"];
        if (maybe_usemvp) {
            g_ApplicationConfig->use_multi_viewport.value = maybe_usemvp.as_boolean();
        }
    }
}
