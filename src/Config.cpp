#include "Config.hpp"

#include "src/os.hpp"
#include "osc_config.hpp"

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
using namespace osc;

namespace {
    std::optional<std::filesystem::path> try_locate_system_config() {
        fs::path p = osc::CurrentExeDir();
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

    void try_update_config_from_config_file(Config& cfg) {
        std::optional<std::filesystem::path> config_pth = try_locate_system_config();

        // can't find underlying config file: warn about it but escape early
        if (!config_pth) {
            log::info("could not find a system configuration file: OSC will still work, but might be missing some configured behavior");
            return;
        }

        // else: can find the config file: try to parse it

        toml::v2::table config;
        try {
            config = toml::parse_file(config_pth->c_str());
        } catch (std::exception const& ex) {
            log::error("error parsing config toml: %s", ex.what());
            log::error("OSC will continue to boot, but you might need to fix your config file (e.g. by deleting it)");
            return;
        }

        // config file parsed as TOML just fine

        // resources
        {
            auto maybe_resources = config["resources"];
            if (maybe_resources) {
                std::string rp = (*maybe_resources.as_string()).get();

                // configuration resource_dir is relative *to the configuration file*
                fs::path config_file_dir = config_pth->parent_path();
                cfg.resourceDir = config_file_dir / rp;
            }
        }

        // docs dir
        if (auto docs = config["docs"]; docs) {
            std::string pth = (*docs.as_string()).get();
            fs::path config_file_dir = config_pth->parent_path();
            cfg.htmlDocsDir = config_file_dir / pth;
        }

        // init `use_multi_viewport`
        {
            auto maybe_usemvp = config["experimental_feature_flags"]["multiple_viewports"];
            if (maybe_usemvp) {
                cfg.useMultiViewport = maybe_usemvp.as_boolean();
            }
        }
    }
}

// public API

std::unique_ptr<Config> osc::Config::load() {
    auto rv = std::make_unique<Config>();

    // set defaults (in case underlying file can't be found)
    rv->resourceDir = OSC_DEFAULT_RESOURCE_DIR;
    rv->useMultiViewport = OSC_DEFAULT_USE_MULTI_VIEWPORT;

    try_update_config_from_config_file(*rv);

    return rv;
}
