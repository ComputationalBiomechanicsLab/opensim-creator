#include "AppConfig.hpp"

#include <oscar/Graphics/AntiAliasingLevel.hpp>
#include <oscar/Platform/AppSettings.hpp>
#include <oscar/Platform/AppSettingValueType.hpp>
#include <oscar/Platform/Log.hpp>
#include <oscar/Platform/LogLevel.hpp>
#include <oscar/Platform/os.hpp>
#include <oscar/Utils/CStringView.hpp>

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

using osc::AntiAliasingLevel;
using osc::AppSettings;
using osc::AppSettingValue;
using osc::AppSettingValueType;
using osc::CStringView;
using osc::CurrentExeDir;
using osc::LogLevel;
using osc::TryParseAsLogLevel;

namespace
{
    constexpr AntiAliasingLevel c_NumMSXAASamples{4};

    std::filesystem::path GetResourcesDirFallbackPath(AppSettings const& settings)
    {
        if (auto const systemConfig = settings.getSystemConfigurationFileLocation())
        {
            auto maybeResourcesPath = systemConfig->parent_path() / "resources";
            if (std::filesystem::exists(maybeResourcesPath))
            {
                return maybeResourcesPath;
            }
            else
            {
                osc::log::warn("resources path fallback: tried %s, but it doesn't exist", maybeResourcesPath.string().c_str());
            }
        }

        auto resourcesRelToExe = CurrentExeDir().parent_path() / "resources";
        if (std::filesystem::exists(resourcesRelToExe))
        {
            return resourcesRelToExe;
        }
        else
        {
            osc::log::warn("resources path fallback: using %s as a filler entry, but it doesn't actaully exist: the application's configuration file has an incorrect/missing 'resources' key", resourcesRelToExe.string().c_str());
        }

        return resourcesRelToExe;
    }

    std::filesystem::path GetResourcesDir(AppSettings const& settings)
    {
        // care: the resources directory is _very_, __very__ imporant
        //
        // if the application can't find resources, then it'll _probably_ fail to
        // boot correctly, which will result in great dissapointment, so this code
        // has to try its best

        constexpr CStringView resourcesKey = "resources";

        auto const resourceDirSettingValue = settings.getValue(resourcesKey);
        if (!resourceDirSettingValue)
        {
            return GetResourcesDirFallbackPath(settings);
        }

        if (resourceDirSettingValue->type() != AppSettingValueType::String)
        {
            osc::log::error("application setting for '%s' is not a string: falling back", resourcesKey.c_str());
            return GetResourcesDirFallbackPath(settings);
        }

        // resolve `resources` dir relative to the configuration file in which it was defined
        std::filesystem::path configFileDir;
        if (auto p = settings.getValueFilesystemSource(resourcesKey))
        {
            configFileDir = p->parent_path();
        }
        else
        {
            configFileDir = CurrentExeDir().parent_path();  // assume the `bin/` dir is one-up from the config
        }
        std::filesystem::path const configuredResourceDir{resourceDirSettingValue->toString()};
        auto resourceDir = std::filesystem::weakly_canonical(configFileDir / configuredResourceDir);

        if (!std::filesystem::exists(resourceDir))
        {
            osc::log::error("'resources', in the application configuration, points to a location that does not exist (%s), so the application may fail to load resources (which is usually a fatal error). Note: the 'resources' path is relative to the configuration file in which you define it (or can be absolute). Attemtping to fallback to a default resources location (which may or may not work).", resourceDir.string().c_str());
            return GetResourcesDirFallbackPath(settings);
        }

        return resourceDir;
    }

    std::filesystem::path GetHTMLDocsDir(AppSettings const& settings)
    {
        constexpr CStringView docsKey = "docs";

        auto const docsSettingValue = settings.getValue(docsKey);
        if (!docsSettingValue)
        {
            // fallback: not set in configuration file
            return std::filesystem::path{};
        }

        if (auto const configLocation = settings.getValueFilesystemSource(docsKey))
        {
            std::filesystem::path const configDir = configLocation->parent_path();
            std::filesystem::path docsLocation = std::filesystem::weakly_canonical(configDir / docsSettingValue->toString());
            if (std::filesystem::exists(docsLocation))
            {
                return docsLocation;
            }
        }

        // fallback: not set, or is set but cannot find it on the filesystem
        return std::filesystem::path{};
    }

    bool GetMultiViewport(AppSettings const& settings)
    {
        return settings
            .getValue("experimental_feature_flags/multiple_viewports")
            .value_or(AppSettingValue{false})
            .toBool();
    }

    std::unordered_map<std::string, bool> GetPanelsEnabledState(AppSettings const&)
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

    std::optional<std::string> GetInitialTab(AppSettings const& settings)
    {
        if (auto v = settings.getValue("initial_tab"))
        {
            return v->toString();
        }
        else
        {
            return std::nullopt;
        }
    }

    LogLevel GetLogLevel(AppSettings const& settings)
    {
        if (auto const v = settings.getValue("log_level"))
        {
            return TryParseAsLogLevel(v->toString()).value_or(LogLevel::DEFAULT);
        }
        else
        {
            return LogLevel::DEFAULT;
        }
    }
}

class osc::AppConfig::Impl final {
public:
    Impl(std::string_view organizationName_,
        std::string_view applicationName_) :
        m_Settings{organizationName_, applicationName_}
    {
    }

    AppSettings m_Settings;
    std::filesystem::path resourceDir = GetResourcesDir(m_Settings);
    std::filesystem::path htmlDocsDir = GetHTMLDocsDir(m_Settings);
    bool useMultiViewport = GetMultiViewport(m_Settings);
    std::unordered_map<std::string, bool> m_PanelsEnabledState = GetPanelsEnabledState(m_Settings);
    std::optional<std::string> m_MaybeInitialTab = GetInitialTab(m_Settings);
    LogLevel m_LogLevel = GetLogLevel(m_Settings);
};

// public API

osc::AppConfig::AppConfig(
    std::string_view organizationName_,
    std::string_view applicationName_) :
    m_Impl{std::make_unique<Impl>(organizationName_, applicationName_)}
{
}

osc::AppConfig::AppConfig(AppConfig&&) noexcept = default;
osc::AppConfig& osc::AppConfig::operator=(AppConfig&&) noexcept = default;
osc::AppConfig::~AppConfig() noexcept = default;

std::filesystem::path osc::AppConfig::getResourcePath(std::string_view k) const
{
    return std::filesystem::weakly_canonical(getResourceDir() / k);
}

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

AntiAliasingLevel osc::AppConfig::getNumMSXAASamples() const
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

LogLevel osc::AppConfig::getRequestedLogLevel() const
{
    return m_Impl->m_LogLevel;
}

std::optional<AppSettingValue> osc::AppConfig::getValue(
    std::string_view key) const
{
    return m_Impl->m_Settings.getValue(key);
}

void osc::AppConfig::setValue(
    std::string_view key,
    AppSettingValue value)
{
    m_Impl->m_Settings.setValue(key, std::move(value));
}
