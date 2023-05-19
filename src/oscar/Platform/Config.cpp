#include "Config.hpp"

#include "oscar/OscarStaticConfig.hpp"
#include "oscar/Platform/Log.hpp"
#include "oscar/Platform/os.hpp"

#include <toml++/toml.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <string>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

static inline int32_t constexpr c_NumMSXAASamples = 4;

namespace
{
    std::optional<std::filesystem::path> TryGetConfigLocation()
    {
        std::filesystem::path p = osc::CurrentExeDir();
        bool exists = false;

        while (p.has_filename())
        {
            std::filesystem::path maybeConfig = p / "osc.toml";
            if (std::filesystem::exists(maybeConfig))
            {
                p = maybeConfig;
                exists = true;
                break;
            }

            // HACK: there is a file at "MacOS/osc.toml", which is where the config
            // is relative to SDL_GetBasePath. current_exe_dir should be fixed
            // accordingly.
            std::filesystem::path maybeMacOSConfig = p / "MacOS" / "osc.toml";
            if (std::filesystem::exists(maybeMacOSConfig))
            {
                p = maybeMacOSConfig;
                exists = true;
                break;
            }
            p = p.parent_path();
        }

        if (exists)
        {
            return p;
        }
        else
        {
            return std::nullopt;
        }
    }

    std::unordered_map<std::string, bool> MakeDefaultPanelStates()
    {
        return
        {
            {"Actions", true},
            {"Navigator", true},
            {"Log", true},
            {"Properties", true},
            {"Selection Details", true},
            {"Simulation Details", false},
            {"Coordinates", true},
            {"Performance", false},
            {"Muscle Plot", false},
            {"Output Watches", false},
            {"Output Plots", true},
        };
    }
}

class osc::Config::Impl final {
public:
    std::filesystem::path resourceDir;
    std::filesystem::path htmlDocsDir;
    bool useMultiViewport;
    std::unordered_map<std::string, bool> m_PanelsEnabledState = MakeDefaultPanelStates();
    std::optional<std::string> m_MaybeInitialTab;
};

namespace
{
    void TryUpdateConfigFromConfigFile(osc::Config::Impl& cfg)
    {
        std::optional<std::filesystem::path> maybeConfigPath = TryGetConfigLocation();

        // can't find underlying config file: warn about it but escape early
        if (!maybeConfigPath)
        {
            osc::log::info("could not find a system configuration file: OSC will still work, but might be missing some configured behavior");
            return;
        }

        // else: can find the config file: try to parse it

        toml::table config;
        try
        {
            config = toml::parse_file(maybeConfigPath->c_str());
        }
        catch (std::exception const& ex)
        {
            osc::log::error("error parsing config toml: %s", ex.what());
            osc::log::error("OSC will continue to boot, but you might need to fix your config file (e.g. by deleting it)");
            return;
        }

        // config file parsed as TOML just fine

        // resources
        {
            auto maybeResourcePath = config["resources"];
            if (maybeResourcePath)
            {
                std::string rp = (*maybeResourcePath.as_string()).get();

                // configuration resource_dir is relative *to the configuration file*
                std::filesystem::path configFileDir = maybeConfigPath->parent_path();
                cfg.resourceDir = configFileDir / rp;
            }
        }

        // docs dir
        if (auto docs = config["docs"]; docs)
        {
            std::string pth = (*docs.as_string()).get();
            std::filesystem::path configFileDir = maybeConfigPath->parent_path();
            cfg.htmlDocsDir = configFileDir / pth;
        }

        // init `initial_tab`
        if (auto initialTabName = config["initial_tab"]; initialTabName)
        {
            cfg.m_MaybeInitialTab = initialTabName.as_string()->get();
        }

        // init `use_multi_viewport`
        {
            auto maybeUseMultipleViewports = config["experimental_feature_flags"]["multiple_viewports"];
            if (maybeUseMultipleViewports)
            {
                cfg.useMultiViewport = maybeUseMultipleViewports.as_boolean();
            }
        }
    }
}

// public API

// try to load the config from disk (default location)
std::unique_ptr<osc::Config> osc::Config::load()
{
    auto rv = std::make_unique<Config::Impl>();

    // set defaults (in case underlying file can't be found)
    rv->resourceDir = OSC_DEFAULT_RESOURCE_DIR;

    TryUpdateConfigFromConfigFile(*rv);

    return std::make_unique<Config>(std::move(rv));
}

osc::Config::Config(std::unique_ptr<Impl> impl) :
    m_Impl{std::move(impl)}
{
}

osc::Config::Config(Config&&) noexcept = default;
osc::Config& osc::Config::operator=(Config&&) noexcept = default;
osc::Config::~Config() noexcept = default;

std::filesystem::path const& osc::Config::getResourceDir() const
{
    return m_Impl->resourceDir;
}

std::filesystem::path const& osc::Config::getHTMLDocsDir() const
{
    return m_Impl->htmlDocsDir;
}

bool osc::Config::isMultiViewportEnabled() const
{
    return m_Impl->useMultiViewport;
}

int32_t osc::Config::getNumMSXAASamples() const
{
    return c_NumMSXAASamples;
}

bool osc::Config::getIsPanelEnabled(std::string const& panelName) const
{
    return m_Impl->m_PanelsEnabledState.try_emplace(panelName, true).first->second;
}

void osc::Config::setIsPanelEnabled(std::string const& panelName, bool v)
{
    m_Impl->m_PanelsEnabledState[panelName] = v;
}

std::optional<std::string> osc::Config::getInitialTabOverride() const
{
    return m_Impl->m_MaybeInitialTab;
}