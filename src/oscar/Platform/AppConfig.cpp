#include "AppConfig.hpp"

#include <oscar/Graphics/AntiAliasingLevel.hpp>
#include <oscar/Platform/Log.hpp>
#include <oscar/Platform/os.hpp>
#include <oscar/Utils/StringHelpers.hpp>
#include <OscarConfiguration.hpp>

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

namespace
{
    constexpr osc::AntiAliasingLevel c_NumMSXAASamples{4};

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

class osc::AppConfig::Impl final {
public:
    std::filesystem::path resourceDir;
    std::filesystem::path htmlDocsDir;
    bool useMultiViewport = false;
    std::unordered_map<std::string, bool> m_PanelsEnabledState = MakeDefaultPanelStates();
    std::optional<std::string> m_MaybeInitialTab;
    LogLevel m_LogLevel = osc::LogLevel::DEFAULT;
};

namespace
{
    void TryUpdateConfigFromConfigFile(osc::AppConfig::Impl& cfg)
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
                std::string rp = maybeResourcePath.as_string()->value_or(cfg.resourceDir.string());

                // configuration resource_dir is relative *to the configuration file*
                std::filesystem::path configFileDir = maybeConfigPath->parent_path();
                cfg.resourceDir = configFileDir / rp;
            }
        }

        // log level
        if (auto maybeLogLevelString = config["log_level"])
        {
            std::string const lvlStr = maybeLogLevelString.as_string()->value_or(std::string{osc::ToCStringView(osc::LogLevel::DEFAULT)});
            if (auto maybeLevel = osc::TryParseAsLogLevel(lvlStr))
            {
                cfg.m_LogLevel = *maybeLevel;
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
                if (auto const* downcasted = maybeUseMultipleViewports.as_boolean())
                {
                    cfg.useMultiViewport = downcasted->get();
                }
                else
                {
                    cfg.useMultiViewport = false;
                }
            }
        }
    }
}

// public API

// try to load the config from disk (default location)
std::unique_ptr<osc::AppConfig> osc::AppConfig::load()
{
    auto rv = std::make_unique<AppConfig::Impl>();

    // set defaults (in case underlying file can't be found)
    rv->resourceDir = OSC_DEFAULT_RESOURCE_DIR;

    TryUpdateConfigFromConfigFile(*rv);

    return std::make_unique<AppConfig>(std::move(rv));
}

osc::AppConfig::AppConfig(std::unique_ptr<Impl> impl) :
    m_Impl{std::move(impl)}
{
}

osc::AppConfig::AppConfig(AppConfig&&) noexcept = default;
osc::AppConfig& osc::AppConfig::operator=(AppConfig&&) noexcept = default;
osc::AppConfig::~AppConfig() noexcept = default;

std::filesystem::path const& osc::AppConfig::getResourceDir() const
{
    return m_Impl->resourceDir;
}

std::filesystem::path const& osc::AppConfig::getHTMLDocsDir() const
{
    return m_Impl->htmlDocsDir;
}

bool osc::AppConfig::isMultiViewportEnabled() const
{
    return m_Impl->useMultiViewport;
}

osc::AntiAliasingLevel osc::AppConfig::getNumMSXAASamples() const
{
    return c_NumMSXAASamples;
}

bool osc::AppConfig::getIsPanelEnabled(std::string const& panelName) const
{
    return m_Impl->m_PanelsEnabledState.try_emplace(panelName, true).first->second;
}

void osc::AppConfig::setIsPanelEnabled(std::string const& panelName, bool v)
{
    m_Impl->m_PanelsEnabledState[panelName] = v;
}

std::optional<std::string> osc::AppConfig::getInitialTabOverride() const
{
    return m_Impl->m_MaybeInitialTab;
}

osc::LogLevel osc::AppConfig::getRequestedLogLevel() const
{
    return m_Impl->m_LogLevel;
}
