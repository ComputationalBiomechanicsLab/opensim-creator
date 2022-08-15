#include "Config.hpp"

#include "src/Platform/Log.hpp"
#include "src/Platform/os.hpp"
#include "osc_config.hpp"

#include <toml.hpp>

#include <algorithm>
#include <chrono>
#include <exception>
#include <filesystem>
#include <fstream>
#include <string>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

static std::optional<std::filesystem::path> TryGetConfigLocation()
{
    fs::path p = osc::CurrentExeDir();
    bool exists = false;

    while (p.has_filename())
    {
        fs::path maybeConfig = p / "osc.toml";
        if (fs::exists(maybeConfig))
        {
            p = maybeConfig;
            exists = true;
            break;
        }

        // HACK: there is a file at "MacOS/osc.toml", which is where the config
        // is relative to SDL_GetBasePath. current_exe_dir should be fixed
        // accordingly.
        fs::path maybeMacOSConfig = p / "MacOS" / "osc.toml";
        if (fs::exists(maybeMacOSConfig))
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

static std::unordered_map<std::string, bool> MakeDefaultPanelStates()
{
    return
    {
        {"Actions", true},
        {"Hierarchy", true},
        {"Log", true},
        {"Property Editor", true},
        {"Selection Details", true},
        {"Simulation Details", false},
        {"Coordinate Editor", true},
        {"Performance", false},
        {"Muscle Plot", false},
        {"Output Watches", false},
        {"Output Plots", true},
    };
}

class osc::Config::Impl final {
public:
    std::filesystem::path resourceDir;
    std::filesystem::path htmlDocsDir;
    bool useMultiViewport;
    static constexpr int numMSXAASamples = 4;
    std::unordered_map<std::string, bool> m_PanelsEnabledState = MakeDefaultPanelStates();
    std::optional<std::string> m_MaybeInitialTab;
};

static void TryUpdateConfigFromConfigFile(osc::Config::Impl& cfg)
{
    std::optional<std::filesystem::path> maybeConfigPath = TryGetConfigLocation();

    // can't find underlying config file: warn about it but escape early
    if (!maybeConfigPath)
    {
        osc::log::info("could not find a system configuration file: OSC will still work, but might be missing some configured behavior");
        return;
    }

    // else: can find the config file: try to parse it

    toml::v2::table config;
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
            fs::path configFileDir = maybeConfigPath->parent_path();
            cfg.resourceDir = configFileDir / rp;
        }
    }

    // docs dir
    if (auto docs = config["docs"]; docs)
    {
        std::string pth = (*docs.as_string()).get();
        fs::path configFileDir = maybeConfigPath->parent_path();
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

// public API

// try to load the config from disk (default location)
std::unique_ptr<osc::Config> osc::Config::load()
{
    auto rv = std::make_unique<Config::Impl>();

    // set defaults (in case underlying file can't be found)
    rv->resourceDir = OSC_DEFAULT_RESOURCE_DIR;
    rv->useMultiViewport = OSC_DEFAULT_USE_MULTI_VIEWPORT;

    TryUpdateConfigFromConfigFile(*rv);

    return std::make_unique<Config>(rv.release());
}

osc::Config::Config(Impl* impl) :
    m_Impl{std::move(impl)}
{
}

osc::Config::Config(Config&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::Config& osc::Config::operator=(Config&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::Config::~Config() noexcept
{
    delete m_Impl;
}

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

int osc::Config::getNumMSXAASamples() const
{
    return m_Impl->numMSXAASamples;
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